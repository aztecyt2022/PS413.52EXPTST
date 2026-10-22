# PS4 HEN 2.2.0 - GitHub Pages host

A basic static PS4 firmware 13.52 host. Enter the configured shared password
before the host controls appear. The standard HEN payload starts after ten
seconds; Disable auto loading cancels this and remembers the choice. Load
exploit launches manually. Other reported devices and firmware versions have
both launch controls disabled.

The System Overlay test is a separate, manual-only option. Its native ShellUI
behavior is unverified on a physical PS4. It attempts to repurpose the former
Friends position as a toggle for the native system-information panel and adds
a second toggle in HEN Settings. Standard HEN remains the default.

## Password and firmware checks

GitHub Pages serves static files and cannot run the previous host's server-side
password checks, HTTP-only sessions, or per-request firmware checks. This copy
has a browser-side password prompt. It is a convenience gate, not secure access
control: visitors can bypass it or fetch any published file directly. All files
in a public repository are public. Do not use this scheme to protect private
data or reuse an account password. The configured password is represented by
a SHA-256 digest in config.js and is never sent to a login server.

Firmware detection uses the reported browser user agent and can be spoofed.
The launcher and direct loader page both require a reported PS4 firmware 13.52.
The original exploit chain and offsets remain unchanged. Both payloads and the
13.52 patch are verified by length and SHA-256 before launch. Kernel exploit
success still needs physical console testing.

## Publish on GitHub Pages

Upload the files in this repository to a public GitHub repository. In repository
Settings > Pages, choose Deploy from a branch, then main and / (root), and Save.
The .nojekyll file disables Jekyll. Relative asset URLs support a project site
such as /hen/ as well as an account site. No Worker, Node server, build service,
or external payload host is required at runtime.

## Local checks

Run npm test, then npm run build. There are no package dependencies.

## Payloads and credits

Standard HEN: Scene-Collective ps4-hen pre-release-main-181, SHA-256
568d57e7c6bfff1b96fc20a4e00b9ca744aa58b135a56eeb5c66c1175acfac3e.

System Overlay test 1: SHA-256
49d372c3eec98c2d308cb8733bebb9a6f73e88cde8297c5fbf12fc895019a214.

Credits: Scene-Collective, illusiony, Al-Azif, raw13g, OSM-Made, and the original
contributors. See LICENSE.loader and overlay-plugin-license.txt. Binaries and
other third-party components retain their original licences.
