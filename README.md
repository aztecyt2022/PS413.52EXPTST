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
