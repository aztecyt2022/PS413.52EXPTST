# User-provided FTPS4 binary — replacement FTP backend

**Selected replacement:** user-uploaded `ps4-ftp(3).bin` (29,052 bytes, SHA-256
`b4b984524ac63508255175c76bdf942ebfbd577234337bfd91e2712e27738560`).

This file is an independent PS4 payload that embeds `FTPS4 Server ready.`,
`Running FTP server`, `Listening on`, `Port: %i`, and firmware/kernel
failure guards. It **is not** the existing HEN `plugin_server.prx` and cannot
be dropped into `/data/hen/plugin_server.prx` or replace a PRX directly.

## Integration requirements

1. Keep the user-supplied input bytes and SHA-256 unchanged.
2. Verify 13.52 compatibility and identify the real FTPS4 runtime port on an
   actual PS4 before showing a port or version as confirmed.
3. Add a native post-jailbreak runner/control channel to start/stop this
   payload without rerunning the exploit or killing shell services.
4. Provide a real FTP-only state independent from plugin enablement. Keep FTP
   off on startup and start FTPS4 only on explicit user activation.
5. Emit the PS4 IP, actual bound port, and the server's *verified* version or
   identity **after successful bind/listen**, with an error notification if
   startup fails.
6. Disable the existing plugin_server FTP listener to avoid a second FTP
   service and port conflicts, without disabling unrelated plugin functions
   such as kernel logging.
7. Perform physical PS4 tests and confirm rollback before replacing the
   verified 13.52 host's native payload.

**Current deployment status:** FTPS4 has been inspected but not yet linked
into the native Settings process. The previously bundled FTP server remains
active as part of the HEN plugin subsystem. The current FTP menu is
informational, not a working FTPS4 post-boot switch. Do not label it otherwise.

The user's original binary is supplied in the ChatGPT conversation and in
the exported FTPS4 test-input bundle. It has **not** been added to this GitHub
repository or installed on a console, so the code must not attempt to fetch it
from a made-up path.
