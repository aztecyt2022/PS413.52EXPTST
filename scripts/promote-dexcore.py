#!/usr/bin/env python3
"""Promote the SHA-verified DEXCore candidate as the default hosted payload.

Called only after both native menu and embedded-code tests. Never overwrite the
known-working HENACY and original HEN binaries.
"""
from pathlib import Path
import hashlib
import re
import shutil

root=Path(__file__).resolve().parents[1]
candidate=root/"dist/dexcore-13.52-test.bin"
published=root/"loader/dexcore.bin"
legacy=root/"loader/henacy.bin"
original=root/"loader/hen.bin"
expected_base="ea1235b846b08434123feec9b9c8e3d16cd9447ec31e0bd115df80589aa42352"
if hashlib.sha256(legacy.read_bytes()).hexdigest()!=expected_base:
    raise SystemExit("Source HENACY firmware payload hash changed; refusing promotion")
if len(candidate.read_bytes())!=500448:
    raise SystemExit("Candidate size changed; refusing promotion")
if hashlib.sha256(legacy.read_bytes()).hexdigest()!="ea1235b846b08434123feec9b9c8e3d16cd9447ec31e0bd115df80589aa42352":
    raise SystemExit("Legacy HENACY rollback binary changed")
if hashlib.sha256(original.read_bytes()).hexdigest()!="568d57e7c6bfff1b96fc20a4e00b9ca744aa58b135a56eeb5c66c1175acfac3e":
    raise SystemExit("Original HEN rollback binary changed")
bytes_=candidate.read_bytes()
assert b"DEXCore v1" in bytes_ and b"FTP Server" in bytes_
assert b" | DEXCore\x00" in bytes_
assert b"DEXCore already loaded. Skipping..." in bytes_
assert b"DEXCore FTP (ftpsrv a717ffd)" in bytes_
assert b"HNCY restarting" not in bytes_
assert b"HENACY already installed" not in bytes_
assert b'title="DEXCore v1" file="hen_settings.xml"' in bytes_
assert b'title="\xe2\x98\x85 HNCY" file="hen_settings.xml"' not in bytes_
shutil.copyfile(candidate,published)
hash_full=hashlib.sha256(bytes_).hexdigest()
hash_short=hash_full[:12]
js="""(function(){
  "use strict";
  /* One current build only: obsolete version/overlay query parameters are ignored. */
  window.HEN_BUILD={
    original:false,
    label:"DEXCore v1",
    query:"",
    payload:["dexcore.bin?v=1-{hash_short}",500448,"{hash_full}"]
  };
})();
""".replace("{hash_short}",hash_short).replace("{hash_full}",hash_full)
(root/"payloads.js").write_text(js)
print("DEXCore is the sole selectable payload; verified source binaries remain internal build inputs.")
print("Promoted SHA256:",hash_full)
