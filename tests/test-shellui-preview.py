"""Offline tests for DEXCore Settings FTP config and safely rebranded candidate."""
import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("shellui_builder", ROOT / "scripts" / "build-shellui-preview.py")
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)


class NativeShellUIMenuTests(unittest.TestCase):
    def test_native_xml_menu_is_structurally_valid(self):
        base = (ROOT / "loader" / "henacy.bin").read_bytes()
        first = builder.XML_START
        last = first + builder.XML_SLOT
        old = base[first:last].decode("utf-8").rstrip()
        edited = (ROOT / "shellui" / "hen_settings.xml").read_text("utf-8")
        builder.validate_xml(old, edited)
        import xml.etree.ElementTree as ET
        root = ET.fromstring(edited)
        nodes = builder.index_ids(root)
        assert nodes["id_hen_settings"].get("title") == "★ DEXCore v1"
        assert nodes["payload_options"].get("initial_focus_to") == "enable_plugins"
        assert "FTP Server" in nodes["payload_options"].get("title")
        switch = nodes["enable_plugins"]
        assert switch.tag == "toggle_switch" and "FTP Server" in switch.get("title")
        assert switch.get("value") == "0" # actual runtime state comes from HEN INI
        assert sum(node.get("id") == "enable_plugins" for node in root.iter()) == 1

    def test_build_modifies_only_xml_resource(self):
        base_path = ROOT / "loader" / "henacy.bin"
        original = base_path.read_bytes()
        with tempfile.TemporaryDirectory() as temp:
            preview = Path(temp) / "candidate.bin"
            builder.build(base_path, ROOT / "shellui" / "hen_settings.xml", preview)
            new = preview.read_bytes()
        self.assertEqual(len(original), len(new))
        at = original.find(builder.TOP_LINK_OLD, 0x29C80, builder.XML_START)
        self.assertGreater(at, 0x29C80)
        self.assertEqual(new[at:at+len(builder.TOP_LINK_OLD)], builder.TOP_LINK_NEW)
        self.assertEqual(original[:at], new[:at])
        self.assertEqual(original[at+len(builder.TOP_LINK_OLD):builder.XML_START],
                         new[at+len(builder.TOP_LINK_OLD):builder.XML_START])
        self.assertNotIn(builder.TOP_LINK_OLD, new[0x29C80:builder.XML_START])
        end = builder.XML_START + builder.XML_SLOT
        old_suffix = builder.OLD_SUFFIX
        offset = original.find(old_suffix, *builder.BRAND_REGION)
        self.assertGreater(offset, end)
        self.assertEqual(original[end:offset], new[end:offset])
        self.assertEqual(new[offset:offset + len(old_suffix)],
                         builder.NEW_SUFFIX.ljust(len(old_suffix), b"\x00"))
        self.assertEqual(original[offset+len(old_suffix):], new[offset+len(old_suffix):])
        self.assertIn(b'DEXCore v1', new[builder.XML_START:end])
        self.assertIn(b'FTP Server', new[builder.XML_START:end])
        self.assertIn(b'enable_plugins', new[builder.XML_START:end])
        self.assertIn(b'hncy_tools', new[builder.XML_START:end])
        self.assertIn(b'hncy_packages', new[builder.XML_START:end])
        self.assertIn(b'hncy_system', new[builder.XML_START:end])
        self.assertNotEqual(original, new)
        self.assertEqual(hashlib.sha256(original).hexdigest(), builder.BASE_SHA256)

    def test_rejects_unrecognized_payload(self):
        with tempfile.TemporaryDirectory() as temp:
            folder = Path(temp)
            corrupt = folder / "corrupt.bin"
            corrupt.write_bytes(b"\x00" * builder.BASE_LENGTH)
            destination = folder / "preview.bin"
            with self.assertRaisesRegex(ValueError, "Unrecognized"):
                builder.build(corrupt, ROOT / "shellui" / "hen_settings.xml", destination)
            self.assertFalse(destination.exists())

    def test_rejects_overwriting_live_payload(self):
        path = ROOT / "loader" / "henacy.bin"
        with self.assertRaisesRegex(ValueError, "overwrite"):
            builder.build(path, ROOT / "shellui" / "hen_settings.xml", path)


if __name__ == "__main__":
    unittest.main()
