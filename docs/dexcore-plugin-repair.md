# Repair the installed DEXCore Settings menu on PS4 13.52

The DEXCore **web page and loader binary are already correctly branded**. The PS4 can continue to show the old menu when its previously stored `/data/hen/plugin_mono.prx` is still loaded. This native PRX **writes** `/data/hen/shellui_data/hen_settings.xml` when it starts, which can undo manual XML-only edits.

The verified repair kit contains an **extracted SELF module with the checked DEXCore root-label modification** from the exact DEXCore 13.52 payload; it does not contain any VSH/XMB overlay. It includes `plugin_mono.prx`, **`plugin_server.prx` (legacy FTP autostart disabled)**, `hen_settings.xml`, `manifest.json`, and the offline FTP repair tool.

## Usage

Connect the PC and PS4 on a **trusted local LAN**. First start a known-working FTP service on the PS4 (the usual HEN server listens on **2121**). Unzip the full kit on the PC.

Using Python 3:

```sh
python repair-dexcore-plugin.py --ip YOUR_PS4_IP --inspect
python repair-dexcore-plugin.py --ip YOUR_PS4_IP --install
```

The repair tool currently installs only the Settings `plugin_mono.prx` and XML; the included `plugin_server.prx` is an independently verified native server replacement. To replace that server, first back up `/data/hen/plugin_server.prx` using FTP on a trusted LAN, then stage/copy the kit’s `plugin_server.prx` to the same path and reboot. Do not overwrite a running program without a recovery backup. The repair tool backs up the original Settings PRX and XML locally, checks the complete downloaded original, uploads the replacement to a temporary name, downloads it again to verify SHA-256, and keeps a remote backup while doing an atomic rename. It **never removes the original backup**. It refuses to install if the existing remote PRX is missing/unreadable or a mismatch suggests a different plugin variant. Read the displayed backup folder and do not delete it.

For rollback, with FTP running:

```sh
python repair-dexcore-plugin.py --ip YOUR_PS4_IP --restore BACKUP_FOLDER
```

**Reboot fully**, then launch DEXCore from the default web host. The original HEN installer can overwrite this PRX on the next boot if the browser loads another HEN build with `upload_prx=1`, so avoid loading the legacy build except for recovery. If HEN says it is already installed, do a complete console restart before testing again.

This plugin-replacement process is console-hardware unverified. Changes to `plugin_mono.prx` can disrupt the Settings page. Keep the original backups and recovery FTP access. The visible top-level Settings link is now **DEXCore v1** as well. Its internal ID remains `hen_settings` for compatibility; internal identifiers are not user-visible.

## Uploaded FTPS4 and old server removal

The *new* DEXCore payload's `plugin_server.prx` does not auto-start the old 2121 FTP thread. Once installed and rebooted, launch the **user-supplied** `ps4-ftp(3).bin` on **1337** from the DEXCore website. If the console has an older native server on disk, that previous PRX may still start FTP on 2121, even though the website changed. Preserve a local backup before updating it. The supplied standalone FTP `.bin` must never replace `plugin_server.prx`.
