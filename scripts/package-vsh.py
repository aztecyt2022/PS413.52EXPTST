#!/usr/bin/env python3
"""Build a self-contained console test kit without replacing the verified 0.1 kit."""
import argparse
import hashlib
import json
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--folder", type=Path, default=root / "builds/vsh-v1.2")
args = parser.parse_args()
folder = args.folder.resolve()
if folder == (root / "builds/vsh").resolve():
    parser.error("Refusing to replace the verified VSH 0.1 package")
manifest = json.loads((folder / "manifest.json").read_text())
if manifest.get("version") != "1.2.0" or manifest.get("platform") != "PS4" or manifest.get("firmware") != "13.52":
    raise SystemExit("Wrong native build/version/firmware: cannot package")
if manifest.get("runtime_tested_on_ps4") is not False:
    raise SystemExit("Release status must not claim unverified console validation")
required = {"henacy_vsh.prx", "henacy-vsh.bin", "plugin_mono_vsh.prx", "plugin_mono_original.prx"}
if set(manifest["files"]) != required:
    raise SystemExit("Missing or unexpected native binaries")
for name, expected in manifest["files"].items():
    data = (folder / name).read_bytes()
    if len(data) != expected["size"] or hashlib.sha256(data).hexdigest() != expected["sha256"]:
        raise SystemExit("Native output does not match manifest: " + name)
destination = folder / "HENACY-VSH-1.2-PS4-Test-Kit.zip"
with ZipFile(destination, "w", compression=ZIP_DEFLATED, compresslevel=9) as archive:
    for name in [*sorted(manifest["files"]), "manifest.json"]:
        archive.write(folder / name, "HENACY-VSH-1.2/" + name)
    for name in ("README.md", "INSTALL.cmd", "RESTORE.cmd", "Install-VSH.ps1"):
        data = (root / "native/vsh" / name).read_bytes()
        if name.endswith(".cmd"):
            data = data.replace(b"\r\n", b"\n").replace(b"\n", b"\r\n")
        archive.writestr("HENACY-VSH-1.2/" + name, data)
print(destination)
