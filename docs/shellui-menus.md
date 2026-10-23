# HENACY v1 — Native PS4 ShellUI menus

This directory is for **PS4 ShellUI/Mono menus, not HTML, GitHub Pages, or XAMPP**.

## Current native Settings integration

The checked `loader/henacy.bin` contains an embedded `plugin_mono` XML resource named `hen_settings.xml`. That resource is rendered inside **PS4 Settings → ★ HNCY** after its plugin has loaded.

Editable menu source: [`shellui/hen_settings.xml`](../shellui/hen_settings.xml).

The proposed native layout is:

```
PS4 Settings
  ★ HNCY
    ★ HENACY v1
      ★ Payload Options (Restart Required)
        Existing HEN configuration toggles (unchanged IDs)
      HENACY Tools
        Plugin Manager
        Package Manager
          Install Package from Hard Disk
          Install Package from USB Storage
      System
        Restart ShellUI
```

Existing toggle IDs, link targets, confirm prompts, and action/button IDs are preserved. This is a **navigation/layout change**, not implementation of new untested system functions.

## Generate a preview without changing the live site

Run from the root of this repository:

```sh
python3 scripts/build-shellui-preview.py
npm run test:shellui
```

The generator reads `loader/henacy.bin` and the editable menu XML. It checks the exact reference SHA-256, the original fixed-size native XML slot, menu IDs/handlers, and that only the XML bytes may differ. The preview is written to `dist/henacy-shellui-preview.bin`. It **never overwrites the original payload**. Python's XML parser and size checks run before output is written.

The preview is **not loaded by the GitHub Pages website** and cannot be assumed to work until tested on a PS4 running the intended firmware. Do not replace an installed plugin or firmware file blindly; keep the original payload and a way to restart ShellUI if testing causes UI instability.

## Scope

This document describes the original HENACY Settings menu and offline XML preview only. The experimental VSH/XMB overlay has been retired; no floating overlay code, controller hook, or VSH test payload is part of the active host.
