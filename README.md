# HENACY v1

**HENACY v1** is the default PS4 firmware **13.52** homebrew host.

**Website:** https://aztecyt2022.github.io/PS413.52EXPTST/

- Launch **HENACY v1** from the main page (default option).
- Select **Original HEN 2.2.0** using the link on the page or `?original=1`.
- Use the browser and host diagnostics page at `loader/diagnostics.html` for read-only compatibility checks.

The HNCY icon is in `assets/hen_settings_icon.png`. The HENACY payload is `loader/henacy.bin`. The original HEN payload is `loader/hen.bin`.

The native System Information branding and its image loading behavior have not been validated on a physical PS4. Browser file-integrity checks do not prove that the exploit will run successfully.

To test the host, run `npm test`. To run the native System Information simulation on a Linux computer with a C compiler, run `npm run test:native`. To build a static copy, run `npm run build`.

## PlayStation Store home-screen tile

A reversible **HENACY v1** replacement for the PlayStation Store home-screen tile is under investigation. This is **not active in the current payload**: the existing `★ HNCY` Settings menu is a separate ShellUI feature. See [the Store tile plan](docs/store-tile-plan.md) and run `python3 scripts/inspect-home-tiles.py /path/to/app.db` to inspect an offline database copy **without changing it**. A real HENACY launcher and a validated restoration path are required before enabling the Store tile override.

## Native ShellUI Settings menu

The PS4 ShellUI menu is now editable in [`shellui/hen_settings.xml`](shellui/hen_settings.xml). The test-only builder [`scripts/build-shellui-preview.py`](scripts/build-shellui-preview.py) creates a separate PS4 native menu preview payload and preserves the current website payload. See [`docs/shellui-menus.md`](docs/shellui-menus.md) for menu layout, safety checks and testing instructions. This is a Settings menu layout revision, **not yet** a global XMB popup; firmware 13.52 runtime testing is still required.
