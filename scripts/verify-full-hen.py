#!/usr/bin/env python3
"""Verify rebuilt HEN embeds all five exact native PRX binaries.

Check before using on 13.52. Hardware behavior cannot be verified in CI.
"""
import hashlib
import json
from pathlib import Path
import sys
if len(sys.argv)!=4:
    raise SystemExit("usage: verify-full-hen.py HEN_BIN PLUGIN_PRX_FOLDER OUTPUT_MANIFEST")
hen=Path(sys.argv[1]).read_bytes()
folder=Path(sys.argv[2])
names=("plugin_bootloader","plugin_loader","plugin_mono","plugin_server","plugin_shellcore")
embedded={}
for name in names:
    data=(folder/f"{name}.prx").read_bytes()
    if not data.startswith(bytes.fromhex("4f153d1d")):
        raise ValueError(f"{name} missing PS4 SELF header")
    matches=[i for i in range(len(hen)) if hen.startswith(data,i)]
    if len(matches)!=1:
        raise ValueError(f"{name} PRX embedded count = {len(matches)} (expected 1)")
    embedded[name]={"bytes":len(data),"sha256":hashlib.sha256(data).hexdigest(),"offset":matches[0]}
if embedded["plugin_mono"]["bytes"]<=141152:
    raise ValueError("Candidate's mono module unexpectedly matches the old fixed-size build")
if not b"DEXCORE" in hen or b"ftp_enabled" not in hen or b"Native HEN FTP" not in hen:
    raise ValueError("Native FTP menu and DEXCORE branding not embedded")
if b"FTPS4: TCP 1337" in hen or b"FTPS4 Server ready" in hen:
    raise ValueError("Standalone FTPS4 appears in the rebuilt native HEN")
if b"DEXCORE %s enabled" not in hen:
    raise ValueError("New installer welcome message missing")
if b"DEXCORE restarting %s" not in hen:
    raise ValueError("Installer restart message missing")
if b" | DEXCORE" not in hen:
    raise ValueError("Custom System Information version label missing")
mono=(folder/"plugin_mono.prx").read_bytes()
for marker in (b"DEXCORE.SystemInformation", b"UISystem", b"BeginRender",
               b"Live overlay: updates every second", b"/data/hen/system_information_enabled"):
    if marker not in mono:
        raise ValueError("Compiled persistent overlay runtime is missing: "+repr(marker))
if b"1.1 TEST 2\0" not in hen:
    raise ValueError("Test build version marker missing")
server=(folder/"plugin_server.prx").read_bytes()
for marker in (b"DEXCORE FTP client connected", b"DEXCORE %s client connected",
               b"DEXCORE BinLoader launched %u bytes", b"DEXCORE Klog enabled\n%s:%u"):
    if marker not in server:
        raise ValueError("Compiled network service or privacy update missing: "+repr(marker))
if b"Client: %s:%u" in server:
    raise ValueError("Client address notification remains in the native server")
# Installer is a flat payload. The host runner checks total size and SHA-256.
result={
    "product":"DEXCORE", "version":"1.1", "fw_target":"13.52",
    "build_variant":"TEST 2",
    "build":"full installer recompiled from Scene-Collective PS4-HEN (not fixed-slot patch)",
    "tested_on_hardware":False, "native_ftp_port":2121,
    "ftp_disabled_on_first_install":True,
    "native_ftp_settings_toggle_included":True,
    "running_native_ftp_verified_on_hardware":False,
    "ftp_requires_plugin_subsystem":True,
    "install_requires_upload_prx_enabled":True,
    "requires_fresh_ps4_boot_for_install":True,
    "standalone_ftps4_embedded":False,
    "system_information_overlay":{
        "native_renderer":"Mono UI2 widgets in UISystem.BeginRender",
        "scenes":["Game container", "active ShellUI scene"],
        "sampler_interval_ms":1000,
        "ui_scene_check_interval_ms":250,
        "off_removes_panels_and_stops_sampling":True,
        "enabled_state_persists":True,
        "console_verified":False
    },
    "payload":{"bytes":len(hen),"sha256":hashlib.sha256(hen).hexdigest()},
    "embedded_prxs":embedded,
}
output=Path(sys.argv[3]);output.parent.mkdir(parents=True,exist_ok=True)
output.write_text(json.dumps(result,indent=2)+"\n")
print(json.dumps(result,indent=2))
