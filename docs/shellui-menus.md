# DEXCore v1 — PS4 13.52 native FTP integration

DEXCore installs its native PS4 Settings entry and standard HEN plugin subsystem. Its native menu's `enable_plugins` switch controls HEN plugins as a group; it is **not** an FTP start switch.

## User-provided FTPS4 is the only active FTP server in new builds

The user provided a standalone 29,052-byte PS4 FTP payload, preserved byte-for-byte and independently verified against SHA-256 `b4b984524ac63508255175c76bdf942ebfbd577234337bfd91e2712e27738560`. It is exposed from `loader/user-ftp.bin`.

Start it **after HEN is loaded** using the DEXCore website's explicit **Launch uploaded FTP (1337)** button. The browser and loader each verify SHA-256 and size before launching the binary. FTPS4 itself notifies the PS4's real IP address and TCP **1337** listening port. This binary does not embed an identifiable semantic software version. Keep the PS4 browser open while using this process-bound server.

DEXCore's `plugin_server.prx` is retained only to preserve HEN's other plugin features. Its compiled call that starts the bundled 2121 FTP thread is disabled through a guarded five-byte patch. The separate kernel log `pthread_create` call remains intact. Do not substitute the standalone `.bin` for a `.prx`: these executables have incompatible formats.

If your PS4 still has an older `/data/hen/plugin_server.prx`, it can continue launching the old 2121 FTP server until you refresh/reinstall the native HEN PRXs and reboot. Backup `/data/hen` before replacing modules.

## Current PS4 Settings and limitations

The existing Settings page uses the original `payload_options` ID, which is important for native `hen.ini` persistence. The FTP entry explains the web-started user FTPS4; the original system and plugin controls remain functional.

There is **no independent native Settings toggle** for this standalone FTPS4 payload yet. The website's FTP launch button is the on-demand control. No native XMB/VSH overlay is installed. Firmware 13.52 hardware validation is still required, and anonymous FTP should run only on a trusted LAN.

Upstream HEN native config: https://github.com/Scene-Collective/ps4-hen ; user payload is shipped from the repository's checksum-verified bundled archive.
