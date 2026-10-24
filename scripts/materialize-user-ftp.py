#!/usr/bin/env python3
"""Reconstruct the user's original PS4 FTP payload byte-for-byte.

Four git-tracked base64 text chunks preserve the 29,052-byte uploaded file in
an xz archive. Refuse to generate/serve anything on any checksum mismatch.
No executable patching or rewriting is performed.
"""
import argparse
import base64
import hashlib
import lzma
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
PARTS=tuple(ROOT/"vendor"/"ps4-ftp"/f"upload-part-{i:02d}.b64" for i in range(1,5))
EXPECTED_XZ_SHA="6799024912a13c9b9a252d0735b62124253fdfb96b879de3e013414af70746ab"
EXPECTED_SHA="b4b984524ac63508255175c76bdf942ebfbd577234337bfd91e2712e27738560"
EXPECTED_SIZE=29052

def materialize(destination: Path):
    encoded="".join(part.read_text("ascii").strip() for part in PARTS)
    if len(encoded)!=16272:
        raise ValueError("FTP archive base64 length mismatch: "+str(len(encoded)))
    archived=base64.b64decode(encoded,validate=True)
    if hashlib.sha256(archived).hexdigest()!=EXPECTED_XZ_SHA:
        raise ValueError("FTP archive digest mismatch")
    data=lzma.decompress(archived,format=lzma.FORMAT_XZ)
    if len(data)!=EXPECTED_SIZE or hashlib.sha256(data).hexdigest()!=EXPECTED_SHA:
        raise ValueError("User-provided PS4 FTP binary digest mismatch")
    if data[0]!=0xe9 or data[0x2872:0x2876]!=(1337).to_bytes(4,"little") or data[0x2926:0x292a]!=(1337).to_bytes(4,"little"):
        raise ValueError("FTP executable format/port mismatch")
    destination.parent.mkdir(parents=True,exist_ok=True)
    if destination.exists() and destination.read_bytes()==data:
        print("FTP payload already matches uploaded file:",destination)
        return
    destination.write_bytes(data)
    print("Restored exact user-uploaded FTP payload:",destination)
    print("  size:",len(data),"sha256:",hashlib.sha256(data).hexdigest(),"port: 1337")

if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output",type=Path,default=ROOT/"loader"/"user-ftp.bin")
    a=parser.parse_args()
    materialize(a.output)
