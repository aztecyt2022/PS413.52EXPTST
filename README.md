# DEXCore v1 — PS4 13.52

**DEXCore** is the new name for the HENACY PS4 13.52 homebrew host.

**Website:** https://aztecyt2022.github.io/PS413.52EXPTST/

The default host loads the actual DEXCore native Settings payload, **not just a renamed web page**. Its previous verified HENACY binary and original HEN remain unmodified for rollback (`?legacy=1` and `?original=1`).

## FTP server in the PS4 Settings menu

The updated native menu is **★ DEXCore v1 → FTP Server / Payload Options → FTP Server + Plugins (Restart Required)**. This is the original HEN `enable_plugins` configuration toggle. On a successfully loaded HEN installation with the bundled server plugin, it enables FTP on **TCP 2121** along with kernel logging and other plugins when HEN is reloaded. It does **not** stop/start FTP immediately or independently. Keep FTP disabled outside a trusted LAN.

The verified DEXCore native menu candidate is now promoted to `loader/dexcore.bin` as the website's default payload. This is not an on-console ShellUI-plugin installer: reboot/reload HEN on PS4 after switching builds. Check the [DEXCore PS4 build artifacts](https://github.com/aztecyt2022/PS413.52EXPTST/actions/workflows/dexcore-native.yml) and [Settings implementation notes](docs/shellui-menus.md). PS4 hardware validation remains necessary.

If your PS4 still displays an old HNCY/HENACY entry, use the [verified on-console plugin repair kit](https://github.com/aztecyt2022/PS413.52EXPTST/actions/workflows/dexcore-plugin-repair.yml). The previous installed `/data/hen/plugin_mono.prx` can retain and regenerate outdated Settings text. The kit backs up that PRX before replacement. Do not install without working recovery access.

The discontinued VSH/XMB overlay is not being restored. If you previously installed a VSH test plugin, restore your console-specific `plugin_mono.prx` backup and reboot before testing DEXCore.
