# DEXCORE 1.1 TEST 2 rollback — PS4 Firmware 13.52

**Website:** https://aztecyt2022.github.io/PS413.52EXPTST/

**TEST 3 withdrawn:** a console report shows ShellUI failing to return after DEXCORE restarts it. The published payload is restored byte for byte to TEST 2 (`8b6788f165058c2e6a6219907f4b0de4fa373c3f09025f98f3f0cd56eb233553`), the preceding build where the home screen returned. The temperature overlay is still unresolved; keep it Off. This rollback needs confirmation on the affected console.

Start from a full PS4 shutdown, clear the PS4 browser cache, then open the website with `?v=test2-recovery1`. The payload notification must say **DEXCORE 1.1 TEST 2 enabled**. `upload_prx = 1` is required to replace TEST 3 plugin files already saved on the console. Do not reload DEXCORE repeatedly within the same boot.

If ShellUI still fails, [the USB recovery configuration](recovery/README.md) bypasses plugin loading and the intentional ShellUI restart. Native builds now run manually and produce downloadable candidates only; CI no longer publishes rebuilt payloads automatically.

The website is a focused **exploit and DEXCORE HEN payload loader**. It retains password authentication, PS4 13.52 firmware checking, payload and kernel-patch SHA-256 verification, manual launch, optional 10-second autoload, and a read-only diagnostic page.

The published `loader/dexcore.bin` is built from the full HEN installer, including all five compiled native plugin modules. The previous fixed-size patch method is archived and cannot replace the current payload.

## Features inside DEXCORE (PS4 Settings)

**DEXCORE → System → System Information** contains the experimental CPU/APU and SoC temperature, fan duty, and estimated RAM overlay controls. The overlay has not displayed on the target console. Leave this control Off during rollback verification; native compilation and automated renderer tests have not established that it works on PS4 13.52.

The console's **DEXCORE → File System → FTP Server** control starts/stops FTP on TCP **2121**. **Network Servers** contains the PS4 LAN address, BinLoader on **9090**, and Klog on **3232**. Each service is independently controlled and defaults Off when its state file is absent. Startup notices show the PS4 server's address; connection notices omit the remote client's IP and port. BinLoader currently supports compatible in-process flat PIC payloads with an `entry(void)` ABI.

No standalone FTPS4 launch button, file browser, or FTP control mechanism is provided by the website.

## Build and verification

- Full HEN build: [rebuild-full-hen.yml](.github/workflows/rebuild-full-hen.yml)
- Native FTP source: [native/ftp-control](native/ftp-control/README.md)
- Current binary size, checksum and embedded modules: [build manifest](builds/dexcore-native-manifest.json)
- Persistent overlay state and UI lifecycle tests: [run_overlay_tests.sh](native/ftp-control/tests/run_overlay_tests.sh)
- Exploit host tests: [tests/host.mjs](tests/host.mjs)
- Native payload manifest and checksums are generated from the built executable, not hardcoded assumptions.

Fully restart the PS4 before loading this test build. The notification should say **DEXCORE 1.1 TEST 2 enabled**. Existing `/data/hen/hen.ini` must retain `enable_plugins = 1` and `upload_prx = 1` to load the new modules. Clear browser cache if the loader reports a checksum mismatch or loads the previous version.

The native build and automated tests do not establish physical PS4 13.52 compatibility. Back up `/data/hen` before the first console test; use the network services only on a trusted LAN.
