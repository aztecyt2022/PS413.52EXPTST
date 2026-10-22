# PROHEN 2.2.0 BETA - GitHub Pages host

A basic static PS4 firmware 13.52 host. The default payload is now PROHEN,
a custom HEN 2.2.0 BETA build. Its PS4 home-menu title is PROHEN. Settings >
System > System Information adds the payload name and version beside the
native firmware text, for example: 13.52 | PROHEN 2.2.0 BETA.

The existing System Overlay addition remains included: the former Friends
position toggles the native system-information panel, with another toggle in
PROHEN Settings. Actual PS4 13.52 behavior remains unverified on a console.
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

PROHEN custom build 2: 517440 bytes, SHA-256
ac645022bf01368f59ebde7bdd17ff8a0c6e2790d12acb72d37ee32d5ccd522a.

Original HEN: Scene-Collective pre-release-main-181, 499776 bytes, SHA-256
568d57e7c6bfff1b96fc20a4e00b9ca744aa58b135a56eeb5c66c1175acfac3e.

PROHEN is a custom fork, not an official upstream release. Full modified source
and the matching compiled plugin are in [the source bundle](source/PROHEN_2.2.0_FW13.52_Source.zip).
Credits: Scene-Collective, illusiony, Al-Azif, raw13g, OSM-Made and the original
contributors. See LICENSE.loader and overlay-plugin-license.txt. All bundled
third-party components retain their licences.
