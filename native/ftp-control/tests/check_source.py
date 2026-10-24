#!/usr/bin/env python3
"""Source integration contract; no unverified PS4 firmware claims."""
from pathlib import Path
from xml.etree import ElementTree
root=Path(__file__).resolve().parents[1]
server=(root/"plugin_server/source/lib.c").read_text()
ftp=(root/"plugin_server/source/ftp/ftp.c").read_text()
settings=(root/"plugin_mono/source/shellui_patch/settings_menu.c").read_text()
menu=ElementTree.parse(root/"hen_settings.xml").getroot()
nodes={n.get("id"):n for n in menu.iter() if n.get("id")}
assert len(nodes)==len([n for n in menu.iter() if n.get("id")])
assert nodes["ftp_enabled"].tag=="toggle_switch"
assert nodes["ftp_enabled"].get("value")=="0"
assert nodes["ftp_enabled"] in list(nodes["ftp_server"])
assert nodes["payload_options"] in list(nodes["hncy_system"])
assert "pthread_ftp_supervisor" in server
assert "pthread_create(&pthrd, 0, dexcore_klog_supervisor, 0)" in server
assert "pthread_create(&pthrd_ftp, 0, pthread_ftp, 0)" not in server
assert "while (ftp_is_active())" in ftp
assert "while (1)" not in ftp[ftp.index("int ftp_main(void)"):]
assert "DEXCORE native FTP enabled" in ftp
assert "DEXCORE FTP client connected" in ftp
assert "Client: %s:%u" not in ftp
assert ftp.index("DEXCORE native FTP enabled")>ftp.index("if (listen(srvfd")
assert '{"KILL", ftp_cmd_unavailable}' in ftp
assert "wSID(L\"ftp_server\")" in settings
assert 'strcmp(Id, "ftp_enabled")' in settings
assert "★ HEN Settings" not in settings
assert "DEXCORE" in settings
assert "DexcoreFtpWriteState" in settings and "DexcoreFtpReadState" in settings
assert "/data/hen/ftp_enabled" in settings
compact=''.join(ElementTree.tostring(menu,encoding='unicode').splitlines())
print("PASS: native FTP files retain independent enable/disable semantics and listening/connection notifications")

mono=(root/"plugin_mono/source/lib.cc").read_text()
assert 'dexcore_overlay_start()' in mono
assert 'dexcore_overlay_ui_install()' in mono
press=settings[settings.index('static void NewOnPress('):settings.index('static void NewOnPreCreate(')]
precreate=settings[settings.index('static void NewOnPreCreate('):settings.index('static void NewOnPageActivating(')]
assert 'dexcore_overlay_ui_queue_toggle(element)' in press
assert press.index('OnPress_Original.ptr(p1,element,p3)') < press.index('dexcore_overlay_ui_queue_toggle(element)')
assert 'mono_object_get_class(element)' in press and 'mono_object_get_class(element)' in precreate
assert 'DexcoreInfoDisplayRow(settings,element,Id)' in precreate
assert 'DexcoreInfoDisplayRow(settings,element,Id)' not in press
ui=(root/'plugin_mono/source/system_overlay_ui.cc').read_text()
assert 'dexcore_overlay_set_enabled' in ui
assert 'system_info_enabled' in nodes and 'network_servers' in nodes
assert 'persistent native overlay' in nodes['system_info_enabled'].get('description')
print('PASS: persistent native overlay sampler, renderer initialization and Settings callback routing are wired')
