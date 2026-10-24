# DEXCore v1 — PS4 13.52

**DEXCore** is the new name for the HENACY PS4 13.52 homebrew host.

**Website:** https://aztecyt2022.github.io/PS413.52EXPTST/

The host offers **one current selectable version: DEXCore v1**. Obsolete version parameters and older-build links no longer select HENACY or original HEN. The existing source binaries are kept as internal build/verification inputs but are not offered in the host menu.

## Native PS4 notifications and menus

The exploit's native installer, ShellUI and System Information HENACY/HNCY notification literals now display DEXCore. The bundled FTP startup notification identifies its actual source revision (`ftpsrv a717ffd`) and displays the PS4 IP, interface, and TCP port **2121**.

The Settings menu now separates **FTP Server**, **Plugins**, and **System → System / Payload Settings**. All existing HEN switches stay under the real native `payload_options` page inside System so that changes still persist to `hen.ini`.

**Runtime FTP limitation:** The bundled server still starts automatically with the HEN plugin subsystem. A separate disabled-by-default FTP service, on-demand startup after boot, and a true FTP-only toggle are **not yet available** in the shipped plugin. The FTP page provides verified server information but does not pretend a cosmetic switch works. A new native server and communication channel are required. The existing FTP banner runs before socket binding, so an IP/port notice does not prove the socket has opened.

See the [native Settings details](docs/shellui-menus.md) and [repair kit](https://github.com/aztecyt2022/PS413.52EXPTST/actions/workflows/dexcore-plugin-repair.yml) if a previously installed module still shows old labels. Hardware testing on PS4 13.52 remains necessary.

The discontinued VSH/XMB overlay is not being restored. `?original=1`, `?legacy=1`, `?henacy=1` and `?vsh=1` now all use DEXCore v1. If you previously installed a VSH test plugin, restore your console-specific `plugin_mono.prx` backup and reboot before testing DEXCore.
