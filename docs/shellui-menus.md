# DEXCore v1 — Native PS4 Settings menu

This is a native PS4 Settings XML menu; **not** a VSH/XMB overlay.

```
PS4 Settings
  ★ HNCY (legacy PS4 native Settings entry)
    ★ DEXCore v1
      ★ FTP Server / Payload Options
        FTP Server + Plugins (Restart Required)
        Existing firmware/payload toggles
      DEXCore Tools
        Plugin Manager
        Package Manager
      System
```

The existing root entry, filenames, internal `hncy_*` IDs and icon paths remain for compatibility with the compiled PS4 Mono hook.

**FTP semantics:** The new visible FTP setting is the existing HEN `enable_plugins` toggle, kept on the `payload_options` page where the upstream ShellUI handler persists it. Enabling plugins allows the bundled HEN `plugin_server` to start FTP on port **2121** and kernel logging on **3232** after HEN is reloaded. It is not a standalone FTP-only toggle, does not report listening state, and is unsuitable for untrusted networks. The server must be installed and load successfully.

Upstream: [HEN configuration](https://github.com/Scene-Collective/ps4-hen/blob/main/installer/source/config.c), [ShellUI write handler](https://github.com/Scene-Collective/ps4-hen-plugins/blob/main/plugin_mono/source/shellui_patch/settings_menu.c), [server plugin](https://github.com/Scene-Collective/ps4-hen-plugins/tree/main/plugin_server/source).

## Native test candidate

Run `python3 scripts/build-shellui-preview.py --output dist/dexcore-13.52-test.bin` to create a separate, SHA-pinned 13.52 candidate. The native version branding is patched in the same candidate under an exact byte check. The original `loader/henacy.bin` is never overwritten. The [DEXCore native CI job](https://github.com/aztecyt2022/PS413.52EXPTST/actions/workflows/dexcore-native.yml) tests and packages the output.

No Sony firmware code, kernel patch or action ID is modified. **Physical PS4 tests are still required.** Keep backup/recovery access before installation.
