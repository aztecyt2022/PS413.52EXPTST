# DEXCORE plugin bypass recovery

Use this only when normal PS4 startup works but loading DEXCORE leaves ShellUI unavailable. TEST 3 is withdrawn. The website currently serves BIN 01, a BIN-only overlay patch based on TEST 2's working startup. Start after a full shutdown and clearing browser cache; the notification must identify `DEXCORE 1.1 BIN 01 enabled`. The exact preceding TEST 2 payload remains available in [the rollback commit](https://github.com/aztecyt2022/PS413.52EXPTST/commit/15eae0cf55560c9658cbb7382e9e95dbace0731c).

The installer source checks `/mnt/usb0/hen.ini` before `/data/hen/hen.ini`. A valid USB configuration is copied to the console. With `config_version = 3`, `enable_plugins = 0` skips installation of the plugin loading hooks and the intentional ShellUI restart. `upload_prx = 0` leaves existing PRX files untouched. This path is verified from installer source, not a physical console test.

1. Keep a copy of your existing `hen.ini`, if available, and any USB `hen.ini` before replacing it. This example uses standard settings; retain any console-specific configuration you require.
2. Save [hen.ini](hen.ini) as `hen.ini` in the root of a USB drive the PS4 mounts as `/mnt/usb0` (not inside a folder, and not `hen.ini.txt`). Use one USB storage device to avoid mount-order ambiguity.
3. Fully shut down and start the PS4. Connect the USB before loading the refreshed website. The recovery settings should leave the normal menu running, with DEXCORE plugins, overlay, and native network services disabled.
4. These settings persist on the console even after unplugging USB. To re-enable the currently published plugins, keep `config_version = 3`, change `upload_prx` and `enable_plugins` to `1` in the USB file, fully shut down again, and load the refreshed website once. This replaces saved PRXs with the modules embedded in the currently published BIN and performs the usual ShellUI restart. Keep the System Information overlay Off while checking menu recovery. To restore the exact TEST 2 modules instead, use the payload from the linked rollback commit with these same installation flags after a full shutdown.

Do not reload HEN repeatedly in the same boot; its installed marker makes subsequent runs skip installation. This recovery does not remove user data or require firmware reinstallation.
