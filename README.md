# HENACY v1

**HENACY v1** is the default PS4 firmware **13.52** homebrew host.

**Website:** https://aztecyt2022.github.io/PS413.52EXPTST/

- Launch **HENACY v1** from the main page (default option).
- Select **Original HEN 2.2.0** using the link on the page or `?original=1`.
- Use the browser and host diagnostics page at `loader/diagnostics.html` for read-only compatibility checks.

The HNCY icon is in `assets/hen_settings_icon.png`. The HENACY payload is `loader/henacy.bin`. The original HEN payload is `loader/hen.bin`.

The native System Information branding and its image loading behavior have not been validated on a physical PS4. Browser file-integrity checks do not prove that the exploit will run successfully.

To test the host, run `npm test`. To run the native System Information simulation on a Linux computer with a C compiler, run `npm run test:native`. To build a static copy, run `npm run build`.
