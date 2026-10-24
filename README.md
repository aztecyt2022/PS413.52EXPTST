# DEXCORE 1.1 TEST 3 — PS4 Firmware 13.52

**Website:** https://aztecyt2022.github.io/PS413.52EXPTST/

The website is a focused **exploit and DEXCORE HEN payload loader**. It retains password authentication, PS4 13.52 firmware checking, payload and kernel-patch SHA-256 verification, manual launch, optional 10-second autoload, and a read-only diagnostic page.

The published `loader/dexcore.bin` is built from the full HEN installer, including all five compiled native plugin modules. The previous fixed-size patch method is archived and cannot replace the current payload.

TEST 3 fixes startup that depended on a later module-load event, binds layout properties using the actual Panel/Label classes, and reads the switch value on the next UI frame after the input event completes. Runtime-resolved render/Settings hooks use relocated five-byte branches instead of requiring fourteen position-independent entry bytes. The status row now starts as **runtime not attached (TEST 3)** and is replaced by the native runtime's actual status.

## Features inside DEXCORE (PS4 Settings)

**DEXCORE → System → System Information → On** enables a persistent native overlay with CPU/APU and SoC temperatures, fan duty percentage, and estimated RAM utilization. It refreshes every second after leaving Settings, follows the active ShellUI scene, and attaches to the Game container for the game overlay. Off removes its panels on the next render frame and stops sensor reads. The setting survives a full console restart. Unsupported sensor APIs display **Unavailable**.

The console's **DEXCORE → File System → FTP Server** control starts/stops FTP on TCP **2121**. **Network Servers** contains the PS4 LAN address, BinLoader on **9090**, and Klog on **3232**. Each service is independently controlled and defaults Off when its state file is absent. Startup notices show the PS4 server's address; connection notices omit the remote client's IP and port. BinLoader currently supports compatible in-process flat PIC payloads with an `entry(void)` ABI.

No standalone FTPS4 launch button, file browser, or FTP control mechanism is provided by the website.

## Build and verification

- Full HEN build: [rebuild-full-hen.yml](.github/workflows/rebuild-full-hen.yml)
- Native FTP source: [native/ftp-control](native/ftp-control/README.md)
- Current binary size, checksum and embedded modules: [build manifest](builds/dexcore-native-manifest.json)
- Persistent overlay state and UI lifecycle tests: [run_overlay_tests.sh](native/ftp-control/tests/run_overlay_tests.sh)
- Exploit host tests: [tests/host.mjs](tests/host.mjs)
- Native payload manifest and checksums are generated from the built executable, not hardcoded assumptions.

Fully restart the PS4 before loading this test build. The notification should say **DEXCORE 1.1 TEST 3 enabled**. Existing `/data/hen/hen.ini` must retain `enable_plugins = 1` and `upload_prx = 1` to load the new modules. Clear browser cache if the loader reports a checksum mismatch or loads the previous version.

The native build and automated tests do not establish physical PS4 13.52 compatibility. Back up `/data/hen` before the first console test; use the network services only on a trusted LAN.
