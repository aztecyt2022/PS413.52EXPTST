# DEXCORE native HEN FTP runtime — source integration candidate

**This is native PRX source, not an activated update to the hosted DEXCORE payload.**
The live host and its installed PS4 PRXs are unchanged until these revised
native modules can be built, installed, and tested with the PS4 toolchain.
Do **not** claim the current PS4 Settings toggle is already operational.

## Implementation

The code is derived from [Scene-Collective/ps4-hen-plugins]
(https://github.com/Scene-Collective/ps4-hen-plugins),
not from GoldHEN's private source. Files keep the upstream license notices;
FTP code and derivatives must be distributed under GPL-3.0-or-later.

- `plugin_server/source/ftp_runtime.c`: runs from native HEN's
  `plugin_server.prx`; missing `/data/hen/ftp_enabled` means **OFF**.
  Watches the file about every 250 ms. Starts and stops the FTP worker without
  stopping the independent kernel-log thread.
- `plugin_server/source/ftp/ftp.c`: original native `ftpsrv` on TCP **2121**;
  bounded accept loop; closes the listening socket and interrupts client
  commands when disabled. Notifies the PS4's IP/port **after** `bind` and
  `listen` succeed; notifies each new client's source IP and port.
  Removes the legacy `KILL` command, which otherwise exits the full plugin.
- `plugin_mono/source/shellui_patch/settings_menu.c`: authentic native
  ShellUI `OnPress` / `OnPreCreate` hooks for the `ftp_enabled` toggle
  on the `ftp_server` page. Persists state with atomic rename.
- `hen_settings.xml`: independent FTP-only toggle under File System, without
  changing the preexisting `enable_plugins` INI property or other actions.

The native FTP server is unauthenticated and highly privileged, so enable it
only on a trusted LAN. Anyone with LAN access may be able to read/write PS4
files. The toggle cannot work when the HEN plugin subsystem itself is off.

## Build integration still required

The embedded 13.52 payload is currently assembled using a SHA-guarded patch
to precompiled `plugin_mono.prx` and `plugin_server.prx`. The new code
**cannot fit into the existing fixed-length ShellUI XML patch alone**.
Build both modified PRXs against a compatible OpenOrbis PS4 toolchain, then
bundle them into the 13.52 HEN installer or deploy with verified on-console
backup/rollback. The original native plugin also contains DEXCORE branding,
an embedded logo, and System Information hooks that must be preserved during
that build. Do not blindly overwrite it with upstream's unbranded PRX.

The GitHub workflow compiles/tests the portable native supervisor using
mock server workers, and checks source integration, **not** complete PS4
PRXs or live firmware 13.52 operation.

The old site FTPS4 payload should be removed only when the compiled native
replacement is ready; disabling it prematurely leaves existing users
without an FTP recovery route.
