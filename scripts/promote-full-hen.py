#!/usr/bin/env python3
"""Promote a verified DEXCORE full HEN candidate into the static loader.

Consumes the checked full-build manifest; never manufactures or pads a payload.
"""
import hashlib
import json
from pathlib import Path
import shutil
import argparse

root=Path(__file__).resolve().parent.parent
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--candidate',type=Path,default=root/"hen/hen.bin")
parser.add_argument('--manifest',type=Path,default=root/"dist/dexcore-full-hen-manifest.json")
args=parser.parse_args()
candidate=args.candidate
manifest=json.loads(args.manifest.read_text())
data=candidate.read_bytes()
digest=hashlib.sha256(data).hexdigest()
expected=manifest["payload"]
assert len(data)==expected["bytes"] and digest==expected["sha256"], "HEN manifest mismatch"
assert data[0]==0xE9, "Unsupported payload entry stub"
assert manifest["product"]=="DEXCORE" and manifest["version"]=="1.1"
assert manifest["fw_target"]=="13.52"
assert manifest["standalone_ftps4_embedded"] is False
assert len(manifest["embedded_prxs"])==5
for prx in manifest["embedded_prxs"].values():
    offset=prx["offset"]
    blob=data[offset:offset+prx["bytes"]]
    assert len(blob)==prx["bytes"]
    assert hashlib.sha256(blob).hexdigest()==prx["sha256"], "Embedded PRX differs"
dest=root/"loader/dexcore.bin"
shutil.copyfile(candidate,dest)
selector='''(function(){
  "use strict";
  window.HEN_BUILD={
    original:false,
    label:"DEXCORE",
    version:"1.1",
    query:"",
    payload:["dexcore.bin?v=1-%s",%d,"%s"]
  };
})();
'''%(digest[:12],len(data),digest)
(root/"payloads.js").write_text(selector)
(root/"builds").mkdir(exist_ok=True)
(root/"builds/dexcore-native-manifest.json").write_text(json.dumps(manifest,indent=2)+"\n")
print("Promoted full DEXCORE HEN:",len(data),digest)
