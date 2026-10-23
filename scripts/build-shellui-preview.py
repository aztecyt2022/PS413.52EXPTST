#!/usr/bin/env python3
"""Build a TEST-ONLY DEXCore PS4 ShellUI menu payload from editable native XML.

Only the verified fixed-size native XML and an exactly matched System Information
version suffix are edited. No firmware data, executable code, or icon is changed.
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
OLD_SUFFIX = b" | HENACY v1\\x00".replace(b"\\\\x00", b"\\x00")
NEW_SUFFIX = b" | DEXCore\\x00"
BRAND_REGION = (0x39A6D, 0x3C000)
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
            if before.get(field) != after.get(field):
                raise ValueError("Existing menu action wiring changed: " + key)
    for key in ("hncy_tools", "hncy_packages", "hncy_system"):
        if key not in current or current[key].tag != "setting_list":
            raise ValueError("Missing native HENACY submenu: " + key)
    if current["id_hen_settings"].get("initial_focus_to") != "payload_options":
        raise ValueError("HENACY initial focus is not connected")


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
    patched_suffix = NEW_SUFFIX.ljust(len(OLD_SUFFIX), b"\\x00")
    candidate[offset:offset + len(OLD_SUFFIX)] = patched_suffix
    # Preserve every byte outside the two approved, non-overlapping ranges.
    assert candidate[:XML_START] == base[:XML_START]
    assert candidate[end:offset] == base[end:offset]
    assert candidate[offset + len(OLD_SUFFIX):] == base[offset + len(OLD_SUFFIX):]
    assert len(candidate) == len(base)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(candidate)
    print("Built DEXCore ShellUI menu TEST payload:", output_path)
    print("SHA-256:", hashlib.sha256(candidate).hexdigest())
    print("XML resource bytes:", len(data), "/", XML_SLOT)
    print("Original web payload, all firmware/kernel bytes, and icon unchanged.")
    print("Native version suffix patched at:", hex(offset))
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
