# HENACY v1 — PS4 13.52

The default website and existing PS4 13.52 payload remain **HENACY v1**.

**Web host:** https://aztecyt2022.github.io/PS413.52EXPTST/

## Native VSH development — HENACY V1.2

The `native/vsh/` source now contains the **V1.2 VHS/XMB menu** based conceptually on Akari v1.2.2 by Jordy Nateur. It adds a VHS Tape theme, 7-item home navigation, screen scan lines, PS4 firmware/temperature information and a Network Services page for FTP (2121), payload (9090) and kernel log (3232).

**Network service start/stop is not yet implemented on PS4.** These controls deliberately report the backend unavailable instead of showing false running status; the HEN plugin's existing daemon services run outside ShellUI. The original VSH 0.1 console binaries and restore kit are left intact. A V1.2 native PRX has been compiled in GitHub Actions, but must be tested on a PS4 before release.

- [Existing VSH 0.1 install kit](https://aztecyt2022.github.io/PS413.52EXPTST/vsh.html)
- [Native VSH V1.2 source, install precautions and credits](native/vsh/README.md)
- [V1.2 native PS4 kit (GitHub Actions artifact)](https://github.com/aztecyt2022/PS413.52EXPTST/actions/workflows/build-henacy-vsh.yml)
- [Host regression checks](https://github.com/aztecyt2022/PS413.52EXPTST/actions/workflows/henacy-vsh.yml)

**PS4 13.52 hardware validation is still required.** Do not treat source changes or passing GitHub Actions as proof the native PRX has been tested on a console.
