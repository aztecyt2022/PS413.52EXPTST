"""Offline tests for the HENACY native ShellUI Settings menu preview builder."""
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

    def test_build_modifies_only_xml_resource(self):
        base_path = ROOT / "loader" / "henacy.bin"
        original = base_path.read_bytes()
        with tempfile.TemporaryDirectory() as temp:
            preview = Path(temp) / "candidate.bin"
            builder.build(base_path, ROOT / "shellui" / "hen_settings.xml", preview)
            new = preview.read_bytes()
        self.assertEqual(len(original), len(new))
        self.assertEqual(original[:builder.XML_START], new[:builder.XML_START])
        end = builder.XML_START + builder.XML_SLOT
        self.assertEqual(original[end:], new[end:])
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
