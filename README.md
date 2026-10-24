# DEXCore v1 — PS4 13.52

**DEXCore** is the new name for the HENACY PS4 13.52 homebrew host.

**Website:** https://aztecyt2022.github.io/PS413.52EXPTST/

The host offers **one current selectable version: DEXCore v1**. Obsolete version parameters and older-build links no longer select HENACY or original HEN. The existing source binaries are kept as internal build/verification inputs but are not offered in the host menu.

## Native DEXCore branding and existing FTP behavior

DEXCore's native installer and System Information notifications are rebranded. The FTP server has been reverted to the original bundled HEN `plugin_server.prx` behavior and its unmodified startup notice; the proposed FTPS4 replacement is not installed or selected.

The PS4 Settings layout is restored to **FTP Server / Payload Options**, **DEXCore Tools**, and **System**. The preexisting **FTP Server + Plugins (Restart Required)** toggle remains a single shared `enable_plugins` setting. It does not independently enable FTP on demand or disable it by default. FTP uses the original server on TCP **2121**, which should only run on a trusted LAN.

The discontinued VSH/XMB overlay is not being restored. `?original=1`, `?legacy=1`, `?henacy=1` and `?vsh=1` now all use DEXCore v1. If you previously installed a VSH test plugin, restore your console-specific `plugin_mono.prx` backup and reboot before testing DEXCore.
