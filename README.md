# DEXCore v1 — PS4 13.52

DEXCore is the PS4 firmware 13.52 homebrew host.

**Website:** https://aztecyt2022.github.io/PS413.52EXPTST/

The website offers only the latest DEXCore v1 payload; old HENACY/HEN binaries are retained internally as verified build source, not available in the menu.

## FTP server: your uploaded file

DEXCore now uses the **exact, unmodified** `ps4-ftp(3).bin` supplied by the user as the on-demand FTP payload.

- Filename served: `loader/user-ftp.bin` (29,052 bytes)
- SHA-256: `b4b984524ac63508255175c76bdf942ebfbd577234337bfd91e2712e27738560`
- Service: FTPS4, **TCP 1337**. This binary has no verified semantic release version string.
- Start: on an already-jailbroken PS4, select **Launch uploaded FTP (1337)** in the DEXCore web host. It verifies the file before launching it; the payload itself displays the PS4 IP and port in a notification after starting.
- Stop: use the FTPS4 `SHUTDOWN` command or end its browser process. Don't expose anonymous FTP to the internet.

The executable is preserved losslessly in `vendor/ps4-ftp/upload-part-*.b64` and reconstructed by `scripts/materialize-user-ftp.py`; every build checks the binary's SHA-256. There is no substitute custom FTP implementation.

**The old bundled HEN `plugin_server.prx` is retained for other HEN plugin features, but its FTP thread startup is disabled** in the current DEXCore native payload by a five-byte, exact-signature-guarded patch. The kernel log thread still launches normally. An older `/data/hen/plugin_server.prx` already installed on your PS4 may nevertheless auto-start the old FTP listener on port **2121**; refresh the native plugins with `upload_prx=1` and a full reboot. Keep a backup of `/data/hen`.

The PS4 Settings FTP page is **informational**. Starting the user-provided FTPS4 server after boot currently uses the website button, **not** a live native Settings toggle. This distinction matters: standalone PS4 shellcode cannot safely replace a PS4 PRX plugin in-place.

See the [native Settings notes](docs/shellui-menus.md) and the [DEXCore plugin repair artifact](https://github.com/aztecyt2022/PS413.52EXPTST/actions/workflows/dexcore-plugin-repair.yml). Hardware behavior on a PS4 13.52 has not yet been verified.

No discontinued VSH/XMB overlay is included.
