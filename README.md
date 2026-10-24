# DEXCORE 1.1 — PS4 13.52

**Live website:** https://aztecyt2022.github.io/PS413.52EXPTST/

DEXCORE 1.1 is a PS4 13.52 homebrew host with a new red, hacker-terminal interface. The public brand is **DEXCORE**; the release number **1.1** is shown separately. The user-provided blue neon DEXCORE PS4 HEN emblem replaces the old H logo on the host and, in an optimized same-size format, in the native PS4 Settings plugin.

## Working host features retained

- Password gate, per-session access and authenticated logout
- Exact PS4 firmware **13.52** detection
- Verified HEN payload, verified kernel patch, manual loading and optional 10-second autoload
- Diagnostics page to check host dependencies without running the exploit
- Verified user-supplied **FTPS4** (29,052-byte `ps4-ftp(3).bin`, SHA-256 `b4b984524ac63508255175c76bdf942ebfbd577234337bfd91e2712e27738560`) launched separately via its **TCP 1337** button

**FTP caveat:** The standalone FTPS4 payload uses the existing exploit runner and has not been validated on physical PS4 firmware 13.52. The bundled native HEN FTP autostart thread has been disabled in the verified native candidate, while kernel logging remains active. If a PS4 has an older plugin already installed, it can continue loading an old FTP service from disk until refreshed. Keep FTP on a trusted LAN and maintain recovery backups.

## GoldHEN-style FTP management (user-provided FTPS4)

GoldHEN has a dedicated FTP on/off setting and normally listens on TCP 2121. DEXCORE keeps the **user-provided FTPS4 binary** (TCP **1337**) unchanged rather than copying GoldHEN's private implementation or silently switching the uploaded FTP file.

- **Off by default:** The built-in HEN FTP autostart is already disabled in the DEXCORE native candidate. Loading the HEN plugin subsystem does not start this server.
- **Enable FTP:** After loading DEXCORE on a PS4 13.52 console, choose **ENABLE FTPS4 :1337** in the DEXCORE website. FTPS4 itself displays the console's IP address and port when it initializes.
- **Status:** On a computer on the same trusted LAN, download `tools/ftp-control.py` from the website and run `python dexcore-ftp-control.py --ip PS4_IP --status`. It checks the live FTP banner, not a stored preference.
- **Disable FTP without a reboot:** Run `python dexcore-ftp-control.py --ip PS4_IP --stop`. The helper verifies `220 FTPS4 Server ready.`, sends FTPS4's documented `SHUTDOWN` command, then checks that the port stops accepting connections. Plugin loading and kernel logging are unaffected.

**Remaining difference from GoldHEN:** The PS4 native Settings page is informational; an independently wired native start/stop toggle is not present. DEXCORE cannot safely send raw FTP commands from ordinary browser JavaScript, so stopping currently needs a LAN computer or other FTP client. This workflow is **not** a native GoldHEN equivalent, and firmware 13.52 hardware testing has not yet confirmed the FTPS4 payload. Keep the browser process alive while using the FTP service.

## PS4 Settings menu

```
DEXCORE
├── File System
│   ├── FTP Server (FTPS4 information; start via website)
│   └── DEXCORE Tools
│       ├── Plugin Manager
│       └── Package Manager
└── System
    ├── System Settings (native payload_options handler)
    │   └── Original HEN switches, exploit patches and Target ID
    └── Restart ShellUI
```

The `payload_options` native ID remains under System so the existing PS4 Settings configuration reads/writes persist. No fake live FTP toggle has been added.

## Native logo and branding

The 128×128 native icon has the same **16,364-byte** allocation and RGBA PNG format as the original asset, verified by SHA-256 guards and build tests. The PS4 Settings root says **DEXCORE**. The System Information version suffix shows ` | DEXCORE` without changing firmware offsets. The website uses a compressed 640×640 WebP made from the supplied image.

## Build and recovery

The original known-good HEN and HENACY binaries remain internal references and are not selectable from the web UI. The [native test build](https://github.com/aztecyt2022/PS413.52EXPTST/actions/workflows/dexcore-native.yml) and [native plugin repair kit](https://github.com/aztecyt2022/PS413.52EXPTST/actions/workflows/dexcore-plugin-repair.yml) are produced by CI. Check [Settings architecture notes](docs/shellui-menus.md).

**Native build and automated checks are not PS4 hardware tests.** If your console still shows older branding, update the installed `/data/hen/plugin_mono.prx` with backup/recovery access and fully reboot before reloading DEXCORE.
