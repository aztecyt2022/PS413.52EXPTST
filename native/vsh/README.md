# HENACY V1.2 — native PS4 development

This is an in-progress x86-64 PS4 ShellUI implementation using **Akari v1.2.2 by Jordy Nateur** as the menu baseline. Akari's PS3 PAF renderer, PowerPC binary, Cobra calls, ID spoofing and PS3 power controls are not used. No Akari source or assets are copied into this port.

**Target: PS4 firmware 13.52 with this repository's HENACY base. Physical PS4 runtime testing is still required.** A successful compile, Linux simulation or website deployment does not certify compatibility on a console. This test build is selectable; it does not replace the website's default payload.

## Menu and controls

Press **L3 + R3** together on the PS4 home screen. Release both after opening. Use **D-pad Up/Down** to navigate, **Left/Right** to change a setting, **Cross** to select and **Circle** to go back. Circle on the main page or L3 + R3 closes the panel. This initial build uses Cross/ Circle directly; regional confirm-button inversion is not implemented.

- **Console manager:** firmware, CPU temperature, SoC sensor, ShellUI update rate and UI canvas dimensions. Missing sensors say `unavailable`. The update counter measures ShellUI callbacks, not game FPS. CPU and SoC readings are sampled once per second; they are not two independent PS4 CPU/GPU sensors.
- **Overlay:** statistics on/off, Celsius/Fahrenheit, left/right placement. It appears on the ShellUI home scene, not inside game processes.
- **Network services:** FTP (2121), payload (9090) and kernel log (3232) status rows. **These are fail-closed until a separately verified PS4 service-control backend is installed**. Selecting a row currently displays an explicit unavailable message and does not falsely claim a server is running. Existing HEN plugin servers may be active independently; this native VSH test module cannot start/stop them yet.
- **Settings:** Crimson, Ocean, Monochrome and VHS Tape themes; opacity from 70–100%; position; save and reset. VHS Tape is the new default, with subtle static scan lines rather than expensive per-frame animation. Both reading and writing use `/data/hen/henacy-vsh.cfg`.
- **About:** Akari attribution and the PS4 SHARE screenshot shortcut.
- **Disable until reboot:** removes widgets and releases input. The small native hooks stay resident and pass input through until ShellUI exits.

The panel is sized against the PS4 UI canvas. Controller input is captured inside ShellUI while the menu is open; the opening/closing button press is drained until release. If ShellUI stops updating for over half a second, input capture is released. Game executables are not patched.

## Install the test kit on Windows

1. Download and fully extract `HENACY-VSH-0.1-PS4-Test-Kit.zip` on the PC. Keep this extracted folder for restoration.
2. Start the known-working HENACY on the PS4. Its plugin system and FTP server must be enabled. The usual FTP port is **2121**. Note the console's LAN IPv4 address.
3. Run `INSTALL.cmd`. Enter that address. For a different FTP port: `INSTALL.cmd -ConsoleIP 192.168.1.50 -Port 2121`.
4. The script backs up an existing `/data/hen/plugin_mono.prx`, then installs and verifies both native modules. It writes only under `/data/hen/`. Do not interrupt the PC or console during the transfer.
5. Reboot the PS4, open `https://aztecyt2022.github.io/PS413.52EXPTST/?vsh=1`, log in and select **Load exploit**. This test selection starts manually.
6. Return to the home screen and press **L3 + R3**. Check the menu and controls below.

The installer needs Windows PowerShell and `curl.exe`. It uploads to temporary names, reads the bytes back and checks SHA-256 before renaming. The Windows installer has been reviewed but has not been exercised against a PS4 FTP server here.

Both files are necessary: `plugin_mono_vsh.prx` is installed as `/data/hen/plugin_mono.prx`, and `henacy_vsh.prx` is installed as `/data/hen/henacy_vsh.prx`. HEN may reuse a plugin already on disk, so downloading only the test `.bin` does not install the menu.

## Restore

While HEN FTP is running, run `RESTORE.cmd` with the same PS4 IP/port. It restores the recorded pre-installation plugin, or the supplied standard HENACY plugin when no backup was recorded. Then **reboot** and use the normal host. Keep the backup folder until the test is finished. Restoring the disk file does not unload callbacks from a running ShellUI process.

Selecting the standard host alone does not reliably restore a stored ShellUI plugin. If ShellUI fails to start, reboot and use your existing recovery/FTP method to restore `backup-IP/plugin_mono.prx` to `/data/hen/plugin_mono.prx`. The supplied `plugin_mono_original.prx` is a fallback for this repository's standard HENACY base. No Sony firmware files need replacing.

## What must be tested on PS4

1. Normal HENACY Settings and System Information still open.
2. The chord opens exactly once while held; the XMB does not move behind the menu.
3. Circle, Cross and D-pad navigate correctly; closing restores XMB input without a stray button press.
4. Opening/closing repeatedly does not duplicate widgets or stop the home screen responding.
5. Settings/Quick Menu, a game transition, user switching, controller disconnect/reconnect and rest mode do not leave input captured.
6. Statistics are plausible; unavailable data is labelled correctly. Check 720p/1080p and the console's actual UI canvas.
7. Save, reboot and verify preferences; restore the prior plugin and confirm the original UI returns.

If the panel does not appear, retrieve `/data/hen/henacy-vsh.log` and the HEN kernel log. The bootstrap also prints its module-load/start result. Missing APIs, unsupported firmware and unsupported detour prologues stop activation; no compatibility offsets are guessed. Once hooks are attempted, the module remains resident even on failure so callbacks cannot point into an unloaded PRX.

## Build and verification

Use OpenOrbis **v0.5.4**, Clang/LLD and a host C compiler, or Zig **0.13.0** for its Clang/LLD tools:

```sh
python3 scripts/build-vsh.py --sdk /path/to/OpenOrbis/PS4Toolchain
# Alternatively: --cc /path/to/zig
npm run test:vsh
python3 scripts/package-vsh.py
npm test
npm run test:native
npm run build
```

The module is a genuine PS4 x86-64 SPRX produced by OpenOrbis `create-fself`, not a renamed PS3 SPRX. A small bootstrap is linked into verified zero padding in the SHA-pinned HENACY `plugin_mono` image. The original System Information boot runs first. Original HEN, default HENACY, the firmware patch region, native menu XML and artwork remain byte-identical in the repository. The bootstrap import bindings are checked against the native NIDs. The separate candidate has its own size/hash checks in the host.

Host checks exercise native menu logic, the PS4 pad structure/filter, nested pad reads, error/disconnect handling, configuration files, actual x86-64 trampoline execution and actual injected bootstrap execution. They also check the firmware/API refusal path, module packaging and website selection. They do not reproduce Sony's PUI renderer or a physical controller.

## Sources and credit

- [Akari 1.2.2 mirror](https://github.com/kernaltrap8/Akari/tree/d264e8bbad882d971a5fab9976fb02af90c36bbc), `Akari/Core/Version.h` and `Akari/Core/Menu/`: menu baseline; original author Jordy Nateur / jordywastaken.
- [OSM-Made/Orbis-Toolbox](https://github.com/OSM-Made/Orbis-Toolbox): public Mono/PUI Application.Update, AreaManager/GetTopScene, Panel, Label, UIFont and UIColor research.
- [OSM-Made/PS4-Mono](https://github.com/OSM-Made/PS4-Mono): public Mono embedding research.
- [Scene-Collective/ps4-hen-plugins](https://github.com/Scene-Collective/ps4-hen-plugins): existing HENACY plugin infrastructure and HDE64 decoder ABI.
- [OpenOrbis v0.5.4](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/releases/tag/v0.5.4): PS4 SPRX packaging and controller/kernel ABI.

## V1.2 progress and known limitation

The original 13.52 payload, the existing VSH PRX/test kit and PS4 firmware offsets have not been replaced with untested output. The V1.2 menu source is updated, but **the downloadable 0.1 binaries must not be called V1.2** until rebuilt with OpenOrbis and tested on a physical PS4. The FTP/payload/klog menu is an honest integration point, not a functional native service controller yet. The original HEN `plugin_server` typically runs FTP and klog in a separate daemon process; these cannot safely be toggled merely by changing a ShellUI boolean. A separate service backend with ownership-aware lifecycle, fault handling and console verification is required before enabling those actions.
