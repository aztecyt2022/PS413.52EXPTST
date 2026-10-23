param(
    [string]$ConsoleIP,
    [ValidateRange(1,65535)][int]$Port = 2121,
    [switch]$Restore
)
$ErrorActionPreference = 'Stop'
try {
    if (-not $ConsoleIP) { $ConsoleIP = Read-Host 'PS4 IPv4 address (HEN FTP must already be running)' }
    $address = $null
    if (-not [System.Net.IPAddress]::TryParse($ConsoleIP, [ref]$address) -or
        $address.AddressFamily -ne [System.Net.Sockets.AddressFamily]::InterNetwork) {
        throw 'Enter a valid PS4 IPv4 address.'
    }
    $ConsoleIP = $address.ToString()
    $curl = (Get-Command curl.exe -ErrorAction Stop).Source
    $manifest = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'manifest.json') -Raw | ConvertFrom-Json
    function Hash([string]$Path) { (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() }
    function Check-Local([string]$Name) {
        $path = Join-Path $PSScriptRoot $Name
        $expected = $manifest.files.PSObject.Properties[$Name].Value
        if (-not $expected -or -not (Test-Path -LiteralPath $path) -or
            (Get-Item -LiteralPath $path).Length -ne $expected.size -or (Hash $path) -ne $expected.sha256) {
            throw "Kit integrity check failed: $Name. Download a fresh kit."
        }
        return $path
    }
    $base = "ftp://${ConsoleIP}:$Port"
    function Fetch([string]$Remote, [string]$Local, [switch]$Optional) {
        & $curl --fail --silent --show-error --connect-timeout 8 --max-time 45 --output $Local "$base$Remote"
        if ($LASTEXITCODE -eq 0) { return $true }
        if ($Optional -and $LASTEXITCODE -eq 78) { return $false }
        throw "FTP download failed ($LASTEXITCODE). Check HEN FTP, the PS4 IP address and port."
    }
    function Put-Verified([string]$Local, [string]$Remote) {
        $staged = "$Remote.vsh-upload"
        & $curl --fail --silent --show-error --connect-timeout 8 --max-time 45 --ftp-create-dirs --upload-file $Local "$base$staged"
        if ($LASTEXITCODE -ne 0) { throw "FTP upload failed ($LASTEXITCODE): $Remote" }
        $verification = Join-Path $env:TEMP ([Guid]::NewGuid().ToString() + '.prx')
        try {
            $null = Fetch $staged $verification
            if ((Hash $Local) -ne (Hash $verification)) { throw "PS4 upload verification failed: $Remote" }
            & $curl --fail --silent --show-error --connect-timeout 8 --max-time 45 --quote "RNFR $staged" --quote "RNTO $Remote" --output NUL "$base/"
            if ($LASTEXITCODE -ne 0) { throw "FTP rename failed ($LASTEXITCODE): the previous $Remote was not replaced." }
        } finally { Remove-Item -LiteralPath $verification -Force -ErrorAction SilentlyContinue }
    }
    $native = Check-Local 'henacy_vsh.prx'
    $bootstrap = Check-Local 'plugin_mono_vsh.prx'
    $original = Check-Local 'plugin_mono_original.prx'
    $null = Check-Local 'henacy-vsh.bin'
    $backup = Join-Path $PSScriptRoot "backup-$ConsoleIP"
    $null = New-Item -ItemType Directory -Path $backup -Force
    $record = Join-Path $backup 'original.json'
    if ($Restore) {
        if (Test-Path -LiteralPath $record) {
            $saved = Get-Content -LiteralPath $record -Raw | ConvertFrom-Json
            $original = Join-Path $backup 'plugin_mono.prx'
            if (-not (Test-Path -LiteralPath $original) -or (Hash $original) -ne $saved.sha256) {
                throw 'Recorded backup is missing or damaged; restore it from your backup copy.'
            }
        } else { Write-Host 'No recorded backup: restoring the supplied standard HENACY ShellUI plugin.' }
        Put-Verified $original '/data/hen/plugin_mono.prx'
        Write-Host 'Standard ShellUI plugin restored. Reboot PS4, then use the normal HENACY host.'
        exit 0
    }
    $current = Join-Path $backup 'current.tmp'
    try {
        $exists = Fetch '/data/hen/plugin_mono.prx' $current -Optional
        if ($exists -and (Hash $current) -ne (Hash $bootstrap) -and -not (Test-Path -LiteralPath $record)) {
            Copy-Item -LiteralPath $current -Destination (Join-Path $backup 'plugin_mono.prx')
            @{ sha256 = (Hash $current); console = $ConsoleIP } | ConvertTo-Json | Set-Content -LiteralPath $record -Encoding ASCII
            Write-Host "Saved the existing ShellUI plugin in $backup"
        }
    } finally { Remove-Item -LiteralPath $current -Force -ErrorAction SilentlyContinue }
    # The bootstrap is installed last. Each upload is hashed before its final rename.
    Put-Verified $native '/data/hen/henacy_vsh.prx'
    Put-Verified $bootstrap '/data/hen/plugin_mono.prx'
    Write-Host 'Both native modules installed and verified.'
    Write-Host 'Reboot PS4. Visit https://aztecyt2022.github.io/PS413.52EXPTST/?vsh=1 and select Load exploit.'
    Write-Host 'On the PS4 home screen, press L3 + R3. Run RESTORE.cmd to restore the previous ShellUI plugin.'
} catch {
    Write-Error $_ -ErrorAction Continue
    exit 1
}
