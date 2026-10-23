import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import unittest
from zipfile import ZipFile

ROOT = Path(__file__).resolve().parents[1]
FOLDER = ROOT / "builds/vsh"


class PackageTests(unittest.TestCase):
    def test_native_manifest_and_ps4_self(self):
        manifest = json.loads((FOLDER / "manifest.json").read_text())
        self.assertEqual(manifest["platform"], "PS4")
        self.assertEqual(manifest["firmware"], "13.52")
        self.assertIs(manifest["runtime_tested_on_ps4"], False)
        for name, spec in manifest["files"].items():
            data = (FOLDER / name).read_bytes()
            self.assertEqual(len(data), spec["size"])
            self.assertEqual(hashlib.sha256(data).hexdigest(), spec["sha256"])
            if not name.endswith(".prx"):
                continue
            self.assertEqual(data[:4], bytes.fromhex("4f153d1d"))
            self.assertEqual(struct.unpack_from("<Q", data, 16)[0], len(data))
            count = struct.unpack_from("<H", data, 24)[0]
            elf = 32 + count * 32
            self.assertEqual(data[elf:elf + 6], b"\x7fELF\x02\x01")
            self.assertEqual(struct.unpack_from("<H", data, elf + 16)[0], 0xFE18)
            self.assertEqual(struct.unpack_from("<H", data, elf + 18)[0], 62)

    def test_candidate_changes_only_bootstrap_and_preserves_native_resources(self):
        base = (ROOT / "loader/henacy.bin").read_bytes()
        candidate = (FOLDER / "henacy-vsh.bin").read_bytes()
        manifest = json.loads((FOLDER / "manifest.json").read_text())
        self.assertEqual(hashlib.sha256(base).hexdigest(), manifest["base_sha256"])
        self.assertEqual(len(base), len(candidate))
        ranges = [(int(a, 16), int(b, 16)) for a, b in manifest["payload_modified_ranges"]]
        self.assertTrue(all(any(a <= i < b for a, b in ranges) for i, (x, y) in enumerate(zip(base, candidate)) if x != y))
        self.assertEqual(base[0xC340:0x143C0], candidate[0xC340:0x143C0])
        self.assertEqual(base[0x386A0:0x39A6D], candidate[0x386A0:0x39A6D])
        self.assertEqual(base[0x424C0:0x464AC], candidate[0x424C0:0x464AC])
        self.assertEqual(candidate[0x29C80:0x4C3E0], (FOLDER / "plugin_mono_vsh.prx").read_bytes())
        self.assertEqual(base[0x29C80:0x4C3E0], (FOLDER / "plugin_mono_original.prx").read_bytes())

    def test_self_contained_kit(self):
        with ZipFile(FOLDER / "HENACY-VSH-0.1-PS4-Test-Kit.zip") as z:
            self.assertIsNone(z.testzip())
            prefix = "HENACY-VSH-0.1/"
            manifest = json.loads(z.read(prefix + "manifest.json"))
            for name, spec in manifest["files"].items():
                self.assertEqual(hashlib.sha256(z.read(prefix + name)).hexdigest(), spec["sha256"])
            for name in ("Install-VSH.ps1", "INSTALL.cmd", "RESTORE.cmd", "README.md"):
                self.assertIn(prefix + name, z.namelist())

    def test_unknown_payload_is_refused(self):
        spec = importlib.util.spec_from_file_location("vsh_build", ROOT / "scripts/build-vsh.py")
        module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
        with self.assertRaises(ValueError):
            module.patch_bootstrap(b"not a PS4 payload", b"")


if __name__ == "__main__":
    unittest.main()
