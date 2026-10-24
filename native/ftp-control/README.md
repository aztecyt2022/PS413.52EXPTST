# DEXCORE 1.1 TEST 3 native plugins

These files overlay the pinned Scene-Collective `ps4-hen-plugins` checkout used by `.github/workflows/rebuild-full-hen.yml`. The workflow compiles all five PRXs and the full HEN installer, verifies each embedded module, and updates the website payload and checksum together. It also records `builds/dexcore-native-manifest.json`.

The persistent System Information overlay uses `plugin_mono/source/system_overlay.c` for a background sample every second. It loads `/data/hen/system_information_enabled` at startup; the native Settings toggle saves that file atomically. Off stops new sampling and invalidates any in-flight result. The UI thread consumes a cached snapshot with a nonblocking lock; it does not read sensor hardware or state files while rendering.

`mono_bootstrap.c` retries against already-loaded assemblies while Mono/app/UI startup completes. It does not wait for another module-load event. The Settings hooks are idempotent, and a visible switch's final value is read on the next UI frame rather than being saved before the input event finishes.

`system_overlay_ui.cc` resolves the loaded Mono UI2 classes and hooks `UISystem.BeginRender`, with `EndRender` as a fallback. It binds properties from the actual Panel/Label classes and creates a panel in the Game container and active ShellUI scene, avoiding duplicate attachment when those roots match. It releases old scene roots and removes panels when disabled. Managed objects are pinned with GC handles; Mono invocation exceptions are caught, and failed operations retry without accumulating panels.

`overlay_hook.c` relocates complete instructions, RIP operands, and relative branches into a nearby relay. `overlay_hook_native.c` installs a five-byte branch without using `MAP_FIXED`. Unsupported relocations leave the method unmodified. The status row reports actual startup/render state; its default XML text does not claim the overlay is live.

UI class and scene attachment conventions follow the primary implementation in [OSM-Made/Orbis-Toolbox](https://github.com/OSM-Made/Orbis-Toolbox), particularly `Game_Overlay.cpp`, `UI_Utilities.cpp`, `Widget.cpp` and `Label.cpp`. Native code offsets are resolved at runtime; no new firmware offsets are supplied.

`system_metrics.c` uses read-only kernel exports when available. CPU/APU, SoC sensors, fan duty and estimated physical RAM usage each have explicit unavailable fallbacks. Fan duty is not RPM, and RAM use includes memory counted as non-free by the kernel.

`plugin_server` retains independent FTP (2121), experimental BinLoader (9090), and Klog (3232) supervisors. Connection notifications are generic and omit remote client addresses. BinLoader accepts compatible in-process flat position-independent payloads with `entry(void)`; it is not a general ELF loader.

Run `bash native/ftp-control/tests/run_overlay_tests.sh` for actual sampler/state tests and a deterministic Mono facade that executes the real renderer's widget, scene, cleanup and exception paths. `tests/check_source.py` also checks that the Settings callbacks are in the correct handlers. These checks and a native compile do not replace physical PS4 13.52 testing.
