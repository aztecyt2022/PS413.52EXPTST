#!/usr/bin/env python3
"""Build a TEST-ONLY DEXCore PS4 ShellUI menu payload from editable native XML.

Only verified static strings, the XML menu, and the System Information suffix change.
The only executable edit is an exact five-byte NOP of the legacy FTP thread creation. No firmware/kernel bytes or kernel log startup instructions are changed.
"""
import argparse
import hashlib
from pathlib import Path
import re
import xml.etree.ElementTree as ET

BASE_SHA256 = "ea1235b846b08434123feec9b9c8e3d16cd9447ec31e0bd115df80589aa42352"
BASE_LENGTH = 500448
XML_START = 0x386A0
XML_SLOT = 5069
# Guarded, shorten-only replacement within the native PUI extension constants.
# Preserve the original NUL-terminated string allocation exactly.
OLD_SUFFIX = b" | HENACY v1\x00"
NEW_SUFFIX = b" | DEXCORE\x00"
BRAND_REGION = (0x39A6D, 0x3C000)

# The outer PS4 Settings link is a separately compiled XML string in the
# plugin_mono PRX. An equal-byte-length swap avoids shifting native rodata.
TOP_LINK_OLD = ('\t\t<link id="hen_settings" title="★ HNCY" file="hen_settings.xml" '
                'icon="file:///data/hen/shellui_data/icons/hen_settings_icon.png" />\r\n').encode("utf-8")
TOP_LINK_HEAD = ('<link id="hen_settings" title="DEXCORE" file="hen_settings.xml" '
                 'icon="file:///data/hen/shellui_data/icons/hen_settings_icon.png" ')
TOP_LINK_NEW = (TOP_LINK_HEAD +
                ' ' * (len(TOP_LINK_OLD) - len((TOP_LINK_HEAD + '/>\r\n').encode("utf-8"))) +
                '/>\r\n').encode("utf-8")
assert len(TOP_LINK_NEW) == len(TOP_LINK_OLD)

# Guarded length-preserving native notification edits. Verified ftpsrv build ID.
NOTIFICATION_PATCHES = (
    ("installer_version", 0xA6AA, b"HENACY v1", b"DEXCORE"),
    ("already_installed", 0xA7D5, b"HENACY already installed. Skipping...", b"DEXCORE already loaded. Skipping..."),
    ("restart_notice", 0xAA2D, b"HNCY restarting %s\nin %d seconds...", b"DEXCORE restart %s\nin %d seconds..."),
    ("shellui_version", 0x376E4, b"HNCY Version: %s", b"DEXCORE: %s"),
    ("topmenu_label", 0x37C56, "★ HNCY".encode("utf-8"), b"DEXCORE"),
    ("info_version", 0x39BA4, b"HENACY v1", b"DEXCORE"),
    ("info_log", 0x39C0D, b"[HNCY] System Information version: %s; logo hook: %s\n", b"[DEXCore] System Information: %s; logo: %s\n"),
)
FTP_BUILD_REVISION = b"a717ffd0b4699"

# In the original bundled plugin_server's plugin_load routine, the first
# pthread_create starts kernel logging; the SECOND starts the old FTP.
# Disable only that call. User-supplied FTPS4 is launched separately by the
# verified web payload runner on demand. Do not touch kernel logging.
BUILTIN_FTP_CREATE_AT = 0x50155
BUILTIN_FTP_CREATE_ORIGINAL = bytes.fromhex("e836180000")
BUILTIN_FTP_CREATE_DISABLED = b"\x90" * 5
BUILTIN_FTP_FUNCTION_AT = 0x4FE60
BUILTIN_FTP_LEA_AT = 0x5014B
BUILTIN_KLOG_CREATE_AT = 0x5013E

# Keep the native writer's original 16,364-byte icon allocation unchanged.
ICON_LEN = 16364
ICON_OLD_SHA256 = "a81ec5f1131b6a4595674b2239429669e10bf3a3fb69ed77e389490c933378ac"
ICON_NEW_SHA256 = "6fb896597b4dbcfdf88595fe12c9d2f14627f455222417f3aceea95493aa2712"
ICON_MAGIC = b"\x89PNG\r\n\x1a\n"




CONFIG_IDS = {"exploit_fixes", "mmap_patches", "block_updates", "disable_aslr",
              "nobd_patches", "target_id", "skip_patches", "upload_prx", "enable_plugins"}
ACTION_IDS = {"id_plugin_manager", "id_game_package_installer_hdd",
              "id_game_package_installer_usb", "id_restart_shellui"}


def index_ids(root):
    found = {}
    for node in root.iter():
        name = node.get("id")
        if name:
            if name in found:
                raise ValueError("Duplicate native menu ID: " + name)
            found[name] = node
    return found


def validate_xml(old_xml, new_xml):
    old = ET.fromstring(old_xml)
    new = ET.fromstring(new_xml)
    if old.tag != "system_settings" or new.tag != "system_settings":
        raise ValueError("Invalid ShellUI root element")
    if old.attrib != new.attrib or new.get("plugin") != "settings_root_plugin":
        raise ValueError("ShellUI plugin contract changed")
    previous, current = index_ids(old), index_ids(new)
    if not previous.keys() <= current.keys():
        raise ValueError("Original Settings elements were removed")
    if not CONFIG_IDS | ACTION_IDS <= current.keys():
        raise ValueError("Missing an existing live Settings action or toggle")
    for key in CONFIG_IDS | ACTION_IDS:
        before, after = previous[key], current[key]
        if before.tag != after.tag:
            raise ValueError("Existing menu action type changed: " + key)
        for field in ("file", "value", "confirm", "confirm_phrase"):
            # Only the enable_plugins explanatory confirmation is shortened to
            # fit native XML space. Its persisted ID/value/action are unchanged.
            if key == "enable_plugins" and field == "confirm":
                continue
            if before.get(field) != after.get(field):
                raise ValueError("Existing menu action wiring changed: " + key)
    for key in ("hncy_tools", "hncy_packages", "hncy_system"):
        if key not in current or current[key].tag != "setting_list":
            raise ValueError("Missing native HENACY submenu: " + key)
    if current["id_hen_settings"].get("initial_focus_to") != "file_system":
        raise ValueError("DEXCORE initial focus must point to File System")
    if current["payload_options"] not in list(current["hncy_system"]):
        raise ValueError("System settings must remain on payload_options handler")
    if current["hncy_tools"] not in list(current["file_system"]):
        raise ValueError("DEXCORE Tools must reside inside File System")


def build(base_path, menu_path, output_path):
    if base_path.resolve() == output_path.resolve():
        raise ValueError("Will not overwrite the active payload")
    base = base_path.read_bytes()
    if len(base) != BASE_LENGTH or hashlib.sha256(base).hexdigest() != BASE_SHA256:
        raise ValueError("Unrecognized HENACY payload; refusing binary edit")
    end = base.find(b"\x00", XML_START)
    if end - XML_START != XML_SLOT:
        raise ValueError("Native ShellUI XML location/size changed")
    original_xml = base[XML_START:end].decode("utf-8").rstrip()
    human_readable = menu_path.read_text("utf-8")
    # Only compress whitespace between tags. Attribute values remain unchanged.
    compact = re.sub(r"<!--.*?-->", "", human_readable, flags=re.S)
    compact = re.sub(r">\s+<", "><", compact).strip()
    validate_xml(original_xml, compact)
    data = compact.encode("utf-8")
    if len(data) > XML_SLOT:
        raise ValueError(f"Menu XML needs {len(data)} bytes; only {XML_SLOT} available")
    candidate = bytearray(base)
    candidate[XML_START:end] = data.ljust(XML_SLOT, b" ")
    # The originally compiled System Information extension uses this exact
    # suffix. Patch only its constant data, not its logic or pointers.
    region_start, region_end = BRAND_REGION
    offset = base.find(OLD_SUFFIX, region_start, region_end)
    if offset < 0 or base.find(OLD_SUFFIX, offset + 1, region_end) >= 0:
        raise ValueError("Unrecognized native System Information brand suffix")
    if not XML_START + XML_SLOT < offset < region_end - len(OLD_SUFFIX):
        raise ValueError("Brand suffix collides with Settings XML or plugin boundary")
    if len(NEW_SUFFIX) > len(OLD_SUFFIX):
        raise ValueError("Replacement text does not fit verified rodata allocation")
    patched_suffix = NEW_SUFFIX.ljust(len(OLD_SUFFIX), b"\x00")
    candidate[offset:offset + len(OLD_SUFFIX)] = patched_suffix
    # The outer PS4 Settings list is not the embedded <setting_list> title.
    # Its precompiled <link> text must be updated separately.
    outer_at = base.find(TOP_LINK_OLD, 0x29C80, XML_START)
    if outer_at < 0 or base.find(TOP_LINK_OLD, outer_at + 1, XML_START) >= 0:
        raise ValueError("Unexpected outer HNCY Settings link (missing or duplicate)")
    if not 0x29C80 < outer_at < XML_START - len(TOP_LINK_OLD):
        raise ValueError("Outer Settings link is outside the plugin PRX")
    candidate[outer_at:outer_at + len(TOP_LINK_OLD)] = TOP_LINK_NEW
    icon_start = base.find(ICON_MAGIC, 0x29C80, 0x4C3E0)
    if icon_start < 0 or base.find(ICON_MAGIC, icon_start + 8, 0x4C3E0) >= 0:
        raise ValueError("Unexpected native icon resource layout")
    original_icon = base[icon_start:icon_start + ICON_LEN]
    if hashlib.sha256(original_icon).hexdigest() != ICON_OLD_SHA256:
        raise ValueError("Original HEN native icon changed")
    user_icon = Path("assets/hen_settings_icon.png").read_bytes()
    if (len(user_icon) != ICON_LEN or not user_icon.startswith(ICON_MAGIC)
        or hashlib.sha256(user_icon).hexdigest() != ICON_NEW_SHA256):
        raise ValueError("Unexpected DEXCORE icon content or size")
    candidate[icon_start:icon_start + ICON_LEN] = user_icon
    # No unapproved firmware or native resource bytes may change.
    allowed = [(icon_start, icon_start + ICON_LEN), (XML_START, end), (offset, offset + len(OLD_SUFFIX)),
               (outer_at, outer_at + len(TOP_LINK_OLD))]
    # Existing plugin_server compiled thread creation:
    #   pthread_create(klog); pthread_create(ftp); while(1) sleep(60);
    # NOP only the second pthread_create. The original immutable payload
    # checksum, function-entry, FTP function pointer LEA and klog call
    # must all agree with the audited bytes before editing code.
    if (base[BUILTIN_KLOG_CREATE_AT:BUILTIN_KLOG_CREATE_AT + 5] != bytes.fromhex("e84d180000")
        or base[BUILTIN_FTP_LEA_AT:BUILTIN_FTP_LEA_AT + 7] != bytes.fromhex("488d150efdffff")
        or base[BUILTIN_FTP_FUNCTION_AT:BUILTIN_FTP_FUNCTION_AT + 4] != bytes.fromhex("554889e5")
        or base[BUILTIN_FTP_CREATE_AT:BUILTIN_FTP_CREATE_AT + 5] != BUILTIN_FTP_CREATE_ORIGINAL):
        raise ValueError("Unrecognized HEN plugin_server FTP thread invocation")
    candidate[BUILTIN_FTP_CREATE_AT:BUILTIN_FTP_CREATE_AT + 5] = BUILTIN_FTP_CREATE_DISABLED
    allowed.append((BUILTIN_FTP_CREATE_AT, BUILTIN_FTP_CREATE_AT + 5))
    for label, at, before, after in NOTIFICATION_PATCHES:
        if len(after) > len(before) or base[at:at+len(before)] != before or base[at+len(before)] != 0:
            raise ValueError("Native notification mismatch: " + label)
        if XML_START <= at < end:
            raise ValueError("Native notification overlaps embedded XML: " + label)
        candidate[at:at+len(before)] = after.ljust(len(before), b"\x00")
        allowed.append((at, at+len(before)))
    allowed.sort()
    for (_, hi), (next_lo, _) in zip(allowed, allowed[1:]):
        if hi > next_lo:
            raise ValueError("Overlapping native data patches")
    cursor = 0
    for lo, hi in allowed:
        assert candidate[cursor:lo] == base[cursor:lo], "Unapproved executable/native modification"
        cursor = hi
    assert candidate[cursor:] == base[cursor:], "Unapproved tail modification"
    assert len(candidate) == len(base)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(candidate)
    print("Built DEXCore ShellUI menu TEST payload:", output_path)
    print("SHA-256:", hashlib.sha256(candidate).hexdigest())
    print("XML resource bytes:", len(data), "/", XML_SLOT)
    print("Native icon updated with equal-size DEXCORE logo; other kernel bytes unchanged.")
    print("Native version suffix patched at:", hex(offset))
    print("Outer PS4 Settings title patched at:", hex(outer_at))
    print("Native notification fixes:", len(NOTIFICATION_PATCHES))
    print("DEXCORE Settings icon bytes:", ICON_LEN)
    print("Builtin plugin_server FTP autostart disabled; klog creation unchanged at:", hex(BUILTIN_KLOG_CREATE_AT))
    print("PS4 13.52 hardware test: NOT YET VERIFIED")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", type=Path, default=Path("loader/henacy.bin"))
    parser.add_argument("--xml", type=Path, default=Path("shellui/hen_settings.xml"))
    parser.add_argument("--output", type=Path, default=Path("dist/dexcore-13.52-test.bin"))
    args = parser.parse_args()
    try:
        build(args.base, args.xml, args.output)
    except (OSError, ValueError, UnicodeError, ET.ParseError) as exc:
        parser.exit(2, "Refusing unsafe preview build: " + str(exc) + "\n")
