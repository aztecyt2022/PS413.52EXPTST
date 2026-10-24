# DEXCORE 1.1 BIN 01 — PS4 Firmware 13.52

**Website:** https://aztecyt2022.github.io/PS413.52EXPTST/

**Current website payload: BIN 01.** This is a direct binary patch of the working TEST 2 installer. It changes 155 bytes to use the concrete Panel/Label classes for layout, explicitly show the text, and set the panel drawing order. CPU/GPU uses the existing CPU/APU temperature reading; fan speed is displayed as fan duty (%). No independent GPU sensor or RPM value is added. Existing startup, sensor sampling and render-hook code are retained.

Start from a full PS4 shutdown, clear the browser cache, and open the website with `?v=dexcore-bin01`. The notification must say **DEXCORE 1.1 BIN 01 enabled**. Keep `enable_plugins = 1` and `upload_prx = 1` in `/data/hen/hen.ini` so the updated module is written and loaded. Enable **DEXCORE → System → System Information** to check the on-screen readouts. Do not reload DEXCORE repeatedly within the same boot.

Desktop tests execute the patched binary's drawing and formatting code, including text updates, scene changes, cleanup, unavailable readings and UI failures. Physical PS4 behavior still needs confirmation. TEST 3 remains withdrawn after its ShellUI restart regression; BIN 01 retains TEST 2's startup path.

The preceding TEST 2 payload is preserved in [the rollback commit](https://github.com/aztecyt2022/PS413.52EXPTST/commit/15eae0cf55560c9658cbb7382e9e95dbace0731c). If ShellUI fails, [the USB recovery configuration](recovery/README.md) bypasses plugin loading and the intentional ShellUI restart. Native builds remain manual candidates and cannot publish rebuilt payloads automatically.

The website is a focused **exploit and DEXCORE HEN payload loader**. It retains password authentication, PS4 13.52 firmware checking, payload and kernel-patch SHA-256 verification, manual launch, optional 10-second autoload, and a read-only diagnostic page.

The published `loader/dexcore.bin` is built from the full HEN installer, including all five compiled native plugin modules. The previous fixed-size patch method is archived and cannot replace the current payload.

## Features inside DEXCORE (PS4 Settings)

**DEXCORE → System → System Information** controls the experimental on-screen CPU/APU and SoC temperatures, fan duty, and estimated RAM readouts. The preceding TEST 2 displayed a grey panel on the target console; BIN 01 targets missing text in that panel. Unsupported readings display **Unavailable**. Enable and disable the overlay to check the updated drawing behavior on the console.

The console's **DEXCORE → File System → FTP Server** control starts/stops FTP on TCP **2121**. **Network Servers** contains the PS4 LAN address, BinLoader on **9090**, and Klog on **3232**. Each service is independently controlled and defaults Off when its state file is absent. Startup notices show the PS4 server's address; connection notices omit the remote client's IP and port. BinLoader currently supports compatible in-process flat PIC payloads with an `entry(void)` ABI.

No standalone FTPS4 launch button, file browser, or FTP control mechanism is provided by the website.

## Build and verification

- Full HEN build: [rebuild-full-hen.yml](.github/workflows/rebuild-full-hen.yml)
- Native FTP source: [native/ftp-control](native/ftp-control/README.md)
- Current binary size, checksum and embedded modules: [build manifest](builds/dexcore-native-manifest.json)
- Persistent overlay state and UI lifecycle tests: [run_overlay_tests.sh](native/ftp-control/tests/run_overlay_tests.sh)
- Exploit host tests: [tests/host.mjs](tests/host.mjs)
- Native payload manifest and checksums are generated from the built executable, not hardcoded assumptions.

Fully restart the PS4 before loading this test build. The notification should say **DEXCORE 1.1 BIN 01 enabled**. Existing `/data/hen/hen.ini` must retain `enable_plugins = 1` and `upload_prx = 1` to load the new modules. Clear browser cache if the loader reports a checksum mismatch or loads the previous version.

The native build and automated tests do not establish physical PS4 13.52 compatibility. Back up `/data/hen` before the first console test; use the network services only on a trusted LAN.
