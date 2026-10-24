# DEXCore v1 native PS4 Settings — FTP status

The native menu has three separate pages: **FTP Server**, **Plugins**, and **System → System / Payload Settings**. The existing system switches remain in a nested page with the compiled ID `payload_options`, because the ShellUI native handler only saves settings under that page ID. Directly moving switches into `hncy_system` would silently prevent them persisting.

The FTP Server page displays the bundled server's identity: `ftpsrv`, revision `a717ffd`, TCP port **2121**. The launch notification from `plugin_server.prx` displays the actual PS4 IP, port and interface. It is emitted before `bind()` succeeds, and is not a socket-state indicator.

**Not implemented:** independent runtime FTP on/off, disabled by default while enabling other plugins, or a post-boot FTP activation event. The built-in FTP service still starts automatically when `enable_plugins=1`. The page intentionally has no fake FTP-only toggle. Implementing one requires modifying both the actual server and native Settings control path.

PS4 installer, ShellUI and System Information branding strings are updated in-place using SHA-verified, length-preserving rodata patches. Compiled firmware/code bytes, native action IDs and icon paths stay unchanged. No VSH menu is restored. Actual PS4 13.52 testing remains required. Use FTP only on a trusted local network; bundled FTP is not password protected.
