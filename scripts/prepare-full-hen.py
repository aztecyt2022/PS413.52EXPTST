#!/usr/bin/env python3
"""Prepare upstream PS4 HEN installer for the DEXCORE 1.1 full-payload build.

Unlike the older fixed-offset binary patch, OpenOrbis/PS4SDK rebuild the
installer with exact newly compiled PRX byte arrays and a new section layout.
"""
from pathlib import Path
import sys

if len(sys.argv)!=2: raise SystemExit("usage: prepare-full-hen.py PATH_TO_PS4_HEN")
root=Path(sys.argv[1])
main=root/"installer/source/main.c"
version=root/"installer/include/version.h"
offsets=root/"kpayload/source/offsets/1352.c"
assert main.exists() and version.exists() and offsets.exists(), "Unexpected HEN source tree"
source=main.read_text()
patches={
    '"HEN is already installed. Skipping..."':'"DEXCORE is already installed. Skipping..."',
    '"Welcome to HEN %s"':'"DEXCORE %s enabled"',
    '"HEN will restart %s\\nin %d seconds..."':'"DEXCORE restarting %s\\nin %d seconds..."',
}
for before,after in patches.items():
    if source.count(before)!=1: raise ValueError(f"Unknown upstream installer notification: {before}")
    source=source.replace(before,after)
main.write_text(source)
header=version.read_text()
before='#define VERSION "2.2.0 BETA"'
assert header.count(before)==1
header=header.replace(before,'#define VERSION "1.1"')
assert '#define MAX_FW 1400' in header and '#define MIN_FW 474' in header
version.write_text(header)
# The native 13.52 offsets are required; never invent kernel addresses.
contents=offsets.read_text()
assert "offsets_1352" in contents and "XFAST_SYSCALL_addr" in contents
config=(root/"hen.ini").read_text()
assert "enable_plugins = 1" in config
assert "upload_prx = 1" in config
print("Prepared DEXCORE full HEN installer with 13.52 offsets; included PRXs are built separately.")
