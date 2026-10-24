# DEXCORE 1.1 — PS4 Firmware 13.52

**Website:** https://aztecyt2022.github.io/PS413.52EXPTST/

The website is a focused **exploit and DEXCORE HEN payload loader**. It retains password authentication, PS4 13.52 firmware checking, payload and kernel-patch SHA-256 verification, manual launch, optional 10-second autoload, and a read-only diagnostic page.

The published `loader/dexcore.bin` is built from the full HEN installer, including all five compiled native plugin modules. The previous fixed-size patch method is archived and cannot replace the current payload.

## Features inside DEXCORE (PS4 Settings)

The console's native **DEXCORE → File System → FTP Server** menu contains the independent FTP enable/disable control, not the website. Native HEN FTP listens on TCP **2121**, is off by default on a fresh install, and includes notifications with listening IP/port and connecting client IP/port. DEXCORE Tools and system settings also live inside the native plugin.

No standalone FTPS4 launch button, file browser, or FTP control mechanism is provided by the website.

## Build and verification

- Full HEN build: [rebuild-full-hen.yml](.github/workflows/rebuild-full-hen.yml)
- Native FTP source: [native/ftp-control](native/ftp-control/README.md)
- Exploit host tests: [tests/host.mjs](tests/host.mjs)
- Native payload manifest and checksums are generated from the built executable, not hardcoded assumptions.

**Hardware warning:** The new native HEN plugins and FTP toggle are compiled and structurally tested but have not been verified on a physical PS4 running firmware 13.52. Preserve a backup of `/data/hen` and a recovery method before testing. FTP is unauthenticated; only enable it on a trusted LAN.
