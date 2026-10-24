# DEXCore v1 — PS4 13.52

**DEXCore** is the new name for the HENACY PS4 13.52 homebrew host.

**Website:** https://aztecyt2022.github.io/PS413.52EXPTST/

The host offers **one current selectable version: DEXCore v1**. Obsolete version parameters and older-build links no longer select HENACY or original HEN. The existing source binaries are kept as internal build/verification inputs but are not offered in the host menu.

## Native DEXCore branding and existing FTP behavior

DEXCore's native installer and System Information notifications are rebranded. The FTP server has been reverted to the original bundled HEN `plugin_server.prx` behavior and its unmodified startup notice; the proposed FTPS4 replacement is not installed or selected.

The PS4 Settings layout is restored to **FTP Server / Payload Options**, **DEXCore Tools**, and **System**. The preexisting **FTP Server + Plugins (Restart Required)** toggle remains a single shared `enable_plugins` setting. It does not independently enable FTP on demand or disable it by default. FTP uses the original server on TCP **2121**, which should only run on a trusted LAN.

The discontinued VSH/XMB overlay is not being restored. `?original=1`, `?legacy=1`, `?henacy=1` and `?vsh=1` now all use DEXCore v1. If you previously installed a VSH test plugin, restore your console-specific `plugin_mono.prx` backup and reboot before testing DEXCore.

## User-uploaded FTPS4 payload (new)

DEXCore includes the **exact unmodified** uploaded `ps4-ftp(3).bin`, SHA-256 `b4b984524ac63508255175c76bdf942ebfbd577234337bfd91e2712e27738560`, 29,052 bytes. The binary is encoded losslessly across four tracked xz+base64 chunks in `vendor/ps4-ftp/` and reconstructed with `python3 scripts/materialize-user-ftp.py` for the GitHub Pages host. The build refuses any checksum mismatch. Its identified listening port is **1337**; its binary doesn't identify a semantic software version.

The site adds an explicit **Launch uploaded FTP (1337)** action. It validates the uploaded binary's SHA-256 before navigating into the existing PS4 payload runner. The runner verifies again before mapping/executing. It can use the existing jailbroken-kernel shortcut; this is **not** an automatic startup service and **not** a native ShellUI toggle. Keep the PS4 browser open while using this FTP payload. Physical PS4 13.52 support is not yet proven for the submitted file.

**Existing installed-plugin caveat:** The new DEXCore payload disables the old FTP autostart when its freshly installed `plugin_server.prx` is loaded. An old `plugin_server.prx` already on PS4 storage may still start its original **2121** FTP listener. Reinstall/refresh HEN plugin PRXs and reboot before testing, keeping backups. Do not overwrite a `.prx` with the standalone `.bin`.

