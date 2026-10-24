#!/usr/bin/env python3
"""Extract a validated DEXCore plugin_mono PRX and its native Settings XML.

The primary HEN payload bundles the PRX in a SELF container. Keeping its
existing SELF envelope is essential; do not extract just the ELF segments.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import struct

ROOT=Path(__file__).resolve().parents[1]
SOURCE_SHA="59189ee8000deb539a58e7a28ec191fc804af145480d3628cf87b1c6c5be1400"
START=0x29C80
END=0x4C3E0
XML_OFFSET=0x386A0
XML_SIZE=5069
SERVER_START=0x4C3E0
SERVER_END=0x5F220
FTP_CREATE_CALL=0x50155
KLOG_CREATE_CALL=0x5013E


def generate(source: Path, output: Path):
    data=source.read_bytes()
    if len(data)!=500448 or hashlib.sha256(data).hexdigest()!=SOURCE_SHA:
        raise ValueError("Unexpected DEXCore payload; native PRX extraction refused")
    # Orbis SELF magic, exact embedded plugin boundaries, and associated ELF.
    if data[START:START+4]!=b"\x4f\x15\x3d\x1d":
        raise ValueError("Embedded native PRX SELF magic missing")
    n=struct.unpack_from("<H",data,START+24)[0]
    elf=START+32+n*32
    if not (0<n<64 and data[elf:elf+4]==b"\x7fELF"):
        raise ValueError("Embedded PRX ELF header invalid")
    plugin=data[START:END]
    # The separate top-level Settings menu label must not remain HNCY.
    if b'title="DEXCORE" file="hen_settings.xml"' not in plugin:
        raise ValueError("Outer PS4 Settings label was not rebranded")
    if b'title="\xe2\x98\x85 HNCY" file="hen_settings.xml"' in plugin:
        raise ValueError("Legacy outer PS4 Settings label still embedded")
    if len(plugin)!=141152 or not plugin[XML_OFFSET-START:XML_OFFSET-START+24].startswith(b"<?xml version"):
        raise ValueError("Native Settings XML is not in expected plugin range")
    raw_xml=data[XML_OFFSET:XML_OFFSET+XML_SIZE].rstrip(b" ")
    if b"DEXCORE" not in raw_xml or b"File System" not in raw_xml:
        raise ValueError("Compiled plugin does not carry the DEXCore FTP menu")
    # Compare the editable XML after its documented, whitespace-only compaction.
    xml=ROOT.joinpath("shellui/hen_settings.xml").read_text("utf-8")
    xml=re.sub(r"<!--.*?-->","",xml,flags=re.S)
    xml=re.sub(r">\s+<","><",xml).strip().encode("utf-8")
    if raw_xml!=xml:
        raise ValueError("Plugin's embedded Settings XML differs from checked source")
    server=data[SERVER_START:SERVER_END]
    if len(server)!=77376 or server[:4]!=bytes.fromhex("4f153d1d"):
        raise ValueError("Invalid plugin_server SELF extraction")
    if data[FTP_CREATE_CALL:FTP_CREATE_CALL+5]!=bytes.fromhex("9090909090"):
        raise ValueError("DEXCore native plugin_server still autostarts old FTP")
    if data[KLOG_CREATE_CALL:KLOG_CREATE_CALL+5]!=bytes.fromhex("e84d180000"):
        raise ValueError("DEXCore native kernel log thread changed")
    output.mkdir(parents=True,exist_ok=True)
    (output/"plugin_mono.prx").write_bytes(plugin)
    (output/"plugin_server.prx").write_bytes(server)
    (output/"hen_settings.xml").write_bytes(raw_xml)
    manifest={
        "platform":"PS4","firmware":"13.52","brand":"DEXCORE","version":"1.1",
        "hardware_verified":False,"target":"/data/hen/plugin_mono.prx",
        "xml_target":"/data/hen/shellui_data/hen_settings.xml",
        "server_target":"/data/hen/plugin_server.prx",
        "user_ftp_port":1337, "legacy_ftp_autostart_disabled":True,
        "files":{name:{"size":len(blob),"sha256":hashlib.sha256(blob).hexdigest()}
                 for name,blob in [("plugin_mono.prx",plugin),("plugin_server.prx",server),("hen_settings.xml",raw_xml)]}}
    (output/"manifest.json").write_text(json.dumps(manifest,indent=2)+"\n")
    # Report only known labels for diagnosis; a web rebrand cannot change
    # text hardcoded in an already installed PS4 plugin.
    for search in (b"HNCY",b"HENACY",b"DEXCore",b"\xe2\x98\x85"):
        offsets=[i for i in range(len(plugin)) if plugin.startswith(search,i)]
        print(f"Plugin literal {search!r}: {len(offsets)} occurrences (first {offsets[:20]})")
        for at in offsets[:5]:
            snippet=plugin[max(0,at-24):at+64].split(b"\x00")[0]
            print("  ",hex(at),repr(snippet))
    print("Validated plugin_mono PRX:",manifest["files"]["plugin_mono.prx"])
    print("Verified plugin_server with old FTP thread disabled:",manifest["files"]["plugin_server.prx"])
    return manifest


if __name__=="__main__":
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--source",type=Path,default=ROOT/"loader/dexcore.bin")
    p.add_argument("--out",type=Path,default=ROOT/"dist/dexcore-plugin-repair")
    args=p.parse_args()
    try:generate(args.source,args.out)
    except (ValueError,OSError) as e:p.exit(2,str(e)+"\n")
