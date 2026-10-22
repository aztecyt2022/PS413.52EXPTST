# PROHEN 2.2.8 BETA - repair candidate 4

A basic static PS4 firmware 13.52 host. The default payload is now PROHEN,
a custom HEN 2.2.0 BETA build. Its PS4 home-menu title is PROHEN. Settings >
System > System Information adds the payload name and version beside the
native firmware text, for example: 13.52 | PROHEN 2.2.8 BETA.

The System Overlay renderer now stays available through PROHEN Settings when
the optional Friends shortcut cannot be installed. The native Friends entry
is preserved during startup; its title changes only after its hook succeeds. Actual PS4 13.52 behavior remains unverified on a console.
Reboot before changing builds; enable_plugins=1 and upload_prx=1 must be set
in the console's existing hen.ini so the matching ShellUI plugin is installed.

Enter the configured shared password before the host controls appear.
Automatic loading starts after ten seconds; Disable auto loading remembers
the choice. Load exploit launches manually. Other reported devices and
firmware versions have both launch controls disabled. Use original HEN opens
?original=1 and selects the unmodified upstream payload. The old ?overlay=1
URL remains a manual-only PROHEN entry for compatibility.

## Password and firmware checks

GitHub Pages serves static files and cannot run the previous host's server-side
password checks, HTTP-only sessions, or per-request firmware checks. This copy
has a browser-side password prompt. It is a convenience gate, not secure access
control: visitors can bypass it or fetch any published file directly. All files
in a public repository are public. The configured password is represented by
a SHA-256 digest in config.js and is never sent to a login server.

Firmware detection uses the browser's reported user agent and can be spoofed.
The launcher and direct loader both require a reported PS4 firmware 13.52.
The original exploit chain and offsets are unchanged; payload selection uses
the shared payloads.js manifest. Payloads and the 13.52 patch are verified by
length and SHA-256 before launch. Exploit success needs physical console testing.

## Publish on GitHub Pages

The repository has index.html at its root. Use main and / (root) for branch-based
GitHub Pages. .nojekyll disables Jekyll. Relative asset URLs support a project
subdirectory. No Worker, Node server, build service, or external payload host
is needed at runtime.

## Local checks

Run npm test and npm run build. There are no package dependencies.

## Payloads and credits

PROHEN repair candidate 4: 587904 bytes, SHA-256
f04082626ee6bae2484e72d9a392f24ab5bcfe007e3793a464ff4ab058b54f4f.

Original HEN: Scene-Collective pre-release-main-181, 499776 bytes, SHA-256
568d57e7c6bfff1b96fc20a4e00b9ca744aa58b135a56eeb5c66c1175acfac3e.

PROHEN is a custom fork, not an official upstream release. Full modified source
and the matching compiled plugin are in [the source bundle](source/PROHEN_2.2.8_FW13.52_Source.zip).
Credits: Scene-Collective, illusiony, Al-Azif, raw13g, OSM-Made and the original
contributors. See LICENSE.loader and overlay-plugin-license.txt. All bundled
third-party components retain their licences.

## Repair and diagnostics

This candidate fixes delayed ShellUI startup, keeps optional Mono exports from
blocking the core menu plugin, preserves the offline PS Plus/PROHEN entry, and
keeps Settings overlay controls independent of the Friends shortcut. A successful
native hook write is checked before startup is marked complete.

After a reboot with enable_plugins=1 and upload_prx=1, expect the notification
"PROHEN build 4: ShellUI plugin loaded." The plugin records setup stages in
/data/hen/prohen-status.log. If the icon or panel still fails, provide that log
and say whether PROHEN Settings exists in Settings. The diagnostic identifies
missing exports, delayed metadata, failed hooks, and panel binding failures.
If no plugin notification or log appears, plugin loading itself still needs
investigation; this candidate does not claim that issue is confirmed fixed.

The actual installed ShellUI plugin path is /data/hen/plugin_mono.prx, as used
by both the installer and kernel loader. Earlier recovery notes naming
plugin_shellui.prx were incorrect. No firmware offsets have been added.

PROHEN 2.2.8 build 4 is prepared for the owner-authorized GitHub Pages
publication. Build and mock checks do not establish real-console success.

## Build 4 optional Mono API repair

The supplied build 3 log confirms a loaded plugin and verified core menu hooks,
but its runtime export lookup could not bind the overlay/version APIs. Build 4
adds an optional plugin_mono_api.prx companion whose imports are resolved by the
PS4 native module linker. The matching payload installs it beside plugin_mono.prx.
It is loaded only as needed; failure leaves the main menu plugin usable and
records an explicit error. The extra APIs remain absent from mandatory imports
in the main plugin. Keep both PRXs from the same build when replacing manually.

After a reboot, expect the build 4 plugin notification and, when the fallback
is used, "14 optional APIs bound by native-link helper" in prohen-status.log.
Physical console testing of this new companion and the UI features is pending.
