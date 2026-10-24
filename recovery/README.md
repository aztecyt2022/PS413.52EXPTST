# DEXCORE plugin bypass recovery

Use this only when normal PS4 startup works but loading DEXCORE leaves ShellUI unavailable. TEST 3 is withdrawn; first try the restored TEST 2 payload after a full shutdown and clearing browser cache. The notification must identify TEST 2.

The installer source checks `/mnt/usb0/hen.ini` before `/data/hen/hen.ini`. A valid USB configuration is copied to the console. With `config_version = 3`, `enable_plugins = 0` skips installation of the plugin loading hooks and the intentional ShellUI restart. `upload_prx = 0` leaves existing PRX files untouched. This path is verified from installer source, not a physical console test.

1. Keep a copy of your existing `hen.ini`, if available, and any USB `hen.ini` before replacing it. This example uses standard settings; retain any console-specific configuration you require.
2. Save [hen.ini](hen.ini) as `hen.ini` in the root of a USB drive the PS4 mounts as `/mnt/usb0` (not inside a folder, and not `hen.ini.txt`). Use one USB storage device to avoid mount-order ambiguity.
3. Fully shut down and start the PS4. Connect the USB before loading the refreshed website. The recovery settings should leave the normal menu running, with DEXCORE plugins, overlay, and native network services disabled.
4. These settings persist on the console even after unplugging USB. To restore the preceding TEST 2 plugins, keep `config_version = 3`, change `upload_prx` and `enable_plugins` to `1` in the USB file, fully shut down again, and load the refreshed website once. This overwrites the saved TEST 3 PRXs with the exact TEST 2 modules and performs the usual ShellUI restart. Keep the System Information overlay Off while checking menu recovery.

Do not reload HEN repeatedly in the same boot; its installed marker makes subsequent runs skip installation. This recovery does not remove user data or require firmware reinstallation.
