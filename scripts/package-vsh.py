#!/usr/bin/env python3
"""Make the self-contained test kit from already verified native outputs."""
import hashlib
import json
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED

root = Path(__file__).resolve().parents[1]
folder = root / "builds/vsh"
manifest = json.loads((folder / "manifest.json").read_text())
for name, expected in manifest["files"].items():
    data = (folder / name).read_bytes()
    if len(data) != expected["size"] or hashlib.sha256(data).hexdigest() != expected["sha256"]:
        raise SystemExit("Native output does not match manifest: " + name)
destination = folder / "HENACY-VSH-0.1-PS4-Test-Kit.zip"
with ZipFile(destination, "w", compression=ZIP_DEFLATED, compresslevel=9) as archive:
    for name in [*manifest["files"], "manifest.json"]:
        archive.write(folder / name, "HENACY-VSH-0.1/" + name)
    for name in ("README.md", "INSTALL.cmd", "RESTORE.cmd", "Install-VSH.ps1"):
        data = (root / "native/vsh" / name).read_bytes()
        if name.endswith(".cmd"):
            data = data.replace(b"\r\n", b"\n").replace(b"\n", b"\r\n")
        archive.writestr("HENACY-VSH-0.1/" + name, data)
print(destination)
