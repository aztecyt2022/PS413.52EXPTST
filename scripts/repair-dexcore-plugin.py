#!/usr/bin/env python3
"""Repair an installed PS4 13.52 DEXCore ShellUI plugin through existing LAN FTP.

Inspection is the default. --install requires a readable, backed-up old module.
All uploads are staged and verified by downloading them again. No /system writes.
"""
import argparse
from datetime import datetime,timezone
import ftplib
import hashlib
import io
import ipaddress
import json
from pathlib import Path
import sys

ROOT=Path(__file__).resolve().parent
PLUGIN="/data/hen/plugin_mono.prx"
XML="/data/hen/shellui_data/hen_settings.xml"


def sha(data):return hashlib.sha256(data).hexdigest()

def get(ftp,path,optional=False):
    out=bytearray()
    try:
        ftp.retrbinary("RETR "+path,out.extend)
    except ftplib.error_perm as e:
        if optional and str(e).startswith(("550","450")):return None
        raise
    return bytes(out)

def put_verified(ftp,path,data):
    stage=path+".dexcore-upload"
    # Clear only an interrupted staging upload, not the active plugin.
    try:ftp.delete(stage)
    except ftplib.error_perm:pass
    ftp.storbinary("STOR "+stage,io.BytesIO(data),blocksize=32768)
    remote=get(ftp,stage)
    if sha(remote)!=sha(data):
        raise RuntimeError("Staged upload verification failed: "+path)
    return stage

def backups(folder,origin,xml):
    folder.mkdir(parents=True,exist_ok=False)
    (folder/"plugin_mono.original.prx").write_bytes(origin)
    if xml is not None:(folder/"hen_settings.original.xml").write_bytes(xml)
    (folder/"backup.json").write_text(json.dumps({
        "plugin_sha256":sha(origin),"plugin_size":len(origin),
        "xml_sha256":sha(xml) if xml is not None else None,
        "remote_plugin":PLUGIN,"remote_xml":XML,
        "created_utc":datetime.now(timezone.utc).isoformat()},indent=2)+"\n")

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--ip",required=True,help="PS4 IPv4 address")
    p.add_argument("--port",type=int,default=2121)
    m=p.add_mutually_exclusive_group(required=True)
    m.add_argument("--inspect",action="store_true")
    m.add_argument("--install",action="store_true")
    m.add_argument("--restore",type=Path,metavar="BACKUP_FOLDER")
    a=p.parse_args()
    address=ipaddress.ip_address(a.ip)
    if address.version!=4 or not address.is_private:
        p.error("Use the PS4's IPv4 address on a private, trusted LAN")
    if not 1<=a.port<=65535:p.error("Invalid FTP port")
    info=json.loads((ROOT/"manifest.json").read_text()) if a.install else None
    if a.install:
        if info["platform"]!="PS4" or info["firmware"]!="13.52" or info["hardware_verified"] is not False:
            p.error("Unrecognized repair manifest")
        new=(ROOT/"plugin_mono.prx").read_bytes()
        for filename,blob in (("plugin_mono.prx",new),("hen_settings.xml",(ROOT/"hen_settings.xml").read_bytes())):
            expected=info["files"][filename]
            if sha(blob)!=expected["sha256"] or len(blob)!=expected["size"]:
                p.error("Repair kit integrity failed for "+filename)
        if len(new)!=141152 or new[:4]!=b"\x4f\x15\x3d\x1d" or b"DEXCore v1" not in new:
            p.error("Invalid extracted native module")
    print("Connecting to %s:%d (FTP has no encryption; trusted LAN only)"%(address,a.port))
    with ftplib.FTP() as ftp:
        ftp.connect(str(address),a.port,timeout=12)
        ftp.login("anonymous","")
        ftp.set_pasv(True)
        current=get(ftp,PLUGIN)
        if not current:raise RuntimeError("Installed plugin_mono.prx not readable. No changes made.")
        if current[:4]!=b"\x4f\x15\x3d\x1d" or not 130000<=len(current)<=170000:
            raise RuntimeError("Unknown installed plugin type/size: no changes made.")
        old_xml=get(ftp,XML,optional=True)
        print("Installed plugin",len(current),sha(current))
        print("Installed menu",None if old_xml is None else (len(old_xml),sha(old_xml)))
        if a.inspect:return
        if a.restore:
            folder=a.restore.resolve()
            backup=json.loads((folder/"backup.json").read_text())
            prev=(folder/"plugin_mono.original.prx").read_bytes()
            if sha(prev)!=backup["plugin_sha256"] or backup["remote_plugin"]!=PLUGIN:
                raise RuntimeError("Backup integrity/path check failed")
            xml_file=folder/"hen_settings.original.xml"
            if backup["xml_sha256"] and (not xml_file.exists() or sha(xml_file.read_bytes())!=backup["xml_sha256"]):
                raise RuntimeError("Original menu XML backup damaged")
            staged=put_verified(ftp,PLUGIN,prev)
            displaced=PLUGIN+".dexcore-before-restore"
            # Keep the current plugin for possible manual recovery too.
            ftp.rename(PLUGIN,displaced)
            try:ftp.rename(staged,PLUGIN)
            except Exception:
                ftp.rename(displaced,PLUGIN)
                raise
            if backup["xml_sha256"]:
                xml_stage=put_verified(ftp,XML,xml_file.read_bytes())
                # Replacing XML is best-effort; the restored plugin re-writes it after reboot.
                try:ftp.delete(XML)
                except ftplib.error_perm:pass
                ftp.rename(xml_stage,XML)
            print("Restored original PRX; reboot before opening Settings.")
            return
        if sha(current)==sha(new):
            print("Installed PRX is already the DEXCore build. Check the displayed top-level name and reboot.")
            return
        folder=ROOT/("backup-ps4-"+str(address)+"-"+datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S"))
        backups(folder,current,old_xml)
        print("Saved original plugin backup at",folder)
        staged=put_verified(ftp,PLUGIN,new)
        # The remote backup allows recovery even if the rename/upload interrupts.
        displaced=PLUGIN+".before-dexcore-"+sha(current)[:12]
        ftp.rename(PLUGIN,displaced)
        try:ftp.rename(staged,PLUGIN)
        except Exception:
            ftp.rename(displaced,PLUGIN)
            raise
        print("DEXCore native PRX installed and verified. Remote backup:",displaced)
        # Only update XML after the plugin replacement succeeded. The new plugin
        # will also write the correct XML when ShellUI loads on reboot.
        xml_data=(ROOT/"hen_settings.xml").read_bytes()
        stage_xml=put_verified(ftp,XML,xml_data)
        try:
            ftp.delete(XML)
        except ftplib.error_perm:
            pass
        ftp.rename(stage_xml,XML)
        print("DEXCore Settings XML installed.")
        print("Power off/reboot PS4 completely and launch default DEXCore (not legacy HENACY).")

if __name__=="__main__":
    try:main()
    except (OSError,RuntimeError,ValueError,ftplib.all_errors) as exc:
        print("FAILED:",exc,file=sys.stderr)
        sys.exit(1)
