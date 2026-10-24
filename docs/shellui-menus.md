# DEXCORE 1.1 — native PS4 Settings and website

The website has a red terminal design displaying the name **DEXCORE** and version **1.1** separately. The original password, exploit, autoload, FTPS4 upload and diagnostic event handlers are kept.

## Native PS4 Settings

```
DEXCORE
├── File System
│   ├── FTP Server (FTPS4: TCP 1337; launched from website)
│   └── DEXCORE Tools
│       ├── Plugin Manager
│       └── Package Manager
└── System
    ├── System Settings [handler ID: payload_options]
    │   ├── Enable Plugins / Exploit Fixes / Memory Mapping
    │   ├── Block Updates / ASLR / NoBD / Target ID
    │   └── Skip Patches / Write PRX Files
    └── Restart ShellUI
```

The native ShellUI settings handler only writes HEN INI values from the page with ID `payload_options`; the entire existing toggle collection remains on that page, now nested under System.

The native Settings root string displays **DEXCORE** rather than its old label. System Information appends ` | DEXCORE`. The 16,364-byte embedded icon is replaced with an equal-length 128×128 RGBA PNG derived from the uploaded image. The PS4 plugin's compiled write_file pointer and byte count remain unchanged. The website uses a 640×640 compressed WebP derived from the same artwork.

The FTPS4 user-supplied executable and its port **1337** remain unchanged. The bundled FTP autostart is disabled in the native test candidate, but existing older plugins on the console may remain active until replaced. The FTP Server Settings page is informational; it is **not** an independent native on/off switch.

The source requires PS4 firmware **13.52**. Build checks are not physical console verification. Back up `/data/hen` before installing a new native plugin.
