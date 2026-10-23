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
expected="edb0fa619aa48de7858fdc1fb2bee30dd9ec6a367fdd28766707e1aac3e9bdae"
if hashlib.sha256(candidate.read_bytes()).hexdigest()!=expected:
    raise SystemExit("Candidate build hash changed; refusing promotion")
if len(candidate.read_bytes())!=500448:
    raise SystemExit("Candidate size changed; refusing promotion")
if hashlib.sha256(legacy.read_bytes()).hexdigest()!="ea1235b846b08434123feec9b9c8e3d16cd9447ec31e0bd115df80589aa42352":
    raise SystemExit("Legacy HENACY rollback binary changed")
if hashlib.sha256(original.read_bytes()).hexdigest()!="568d57e7c6bfff1b96fc20a4e00b9ca744aa58b135a56eeb5c66c1175acfac3e":
    raise SystemExit("Original HEN rollback binary changed")
bytes_=candidate.read_bytes()
assert b"DEXCore v1" in bytes_ and b"FTP Server" in bytes_
assert b" | DEXCore\x00" in bytes_
shutil.copyfile(candidate,published)
js="""(function(){
  "use strict";
  var query=location.search||"";
  var legacy=/(?:[?&])legacy=1(?:&|$)/.test(query);
  var henacy=/(?:[?&])henacy=1(?:&|$)/.test(query);
  var original=!legacy&&!henacy&&/(?:[?&])original=1(?:&|$)/.test(query);
  /* No VSH overlay. ?legacy=1 is the immutable HENACY rollback binary. */
  window.HEN_BUILD={
    original:original,
    legacy:legacy,
    label:original?"PS4 HEN 2.2.0":legacy?"HENACY v1 (legacy)":"DEXCore v1",
    query:original?"?original=1":legacy?"?legacy=1":"",
    payload:original?
      ["hen.bin",499776,"568d57e7c6bfff1b96fc20a4e00b9ca744aa58b135a56eeb5c66c1175acfac3e"]:
      legacy?["henacy.bin?v=1-syslogo-ea1235b846b0",500448,"ea1235b846b08434123feec9b9c8e3d16cd9447ec31e0bd115df80589aa42352"]:
      ["dexcore.bin?v=1-edb0fa619aa4",500448,"edb0fa619aa48de7858fdc1fb2bee30dd9ec6a367fdd28766707e1aac3e9bdae"]
  };
})();
"""
(root/"payloads.js").write_text(js)
print("DEXCore is now the default payload; legacy and original payload binaries preserved.")
print("Promoted SHA256:",hashlib.sha256(published.read_bytes()).hexdigest())
