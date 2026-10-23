# PS4 13.52 host

Select a payload using the links on the host page:

| Payload | Selection |
| --- | --- |
| HENACY v1 | Default |
| Original PS4 HEN 2.2.0 | `?original=1` |

HENACY v1 uses the **HNCY** PS4 settings-menu label and the red HNCY emblem.
The 128×128 RGBA menu image is included at `assets/hen_settings_icon.png` and
embedded in the menu plugin. Loading HENACY writes it to
`/data/hen/shellui_data/icons/hen_settings_icon.png`.

In **Settings → System → System Information**, the System Software row includes
the native firmware number followed by **HENACY v1**, with the HNCY logo beside
the text. The display extension resolves the managed UI classes at runtime. If
the optional image loader is unavailable, the version text remains usable.
The console firmware value itself is unchanged.

Its payload is stored at
`loader/henacy.bin` (500,448 bytes), with SHA-256:

```
529a4fbb76f59bfc87917bc8cdd4401289e06734e3d2cbfce6170d754c71c427
```

The host verifies payload size and SHA-256 before launching. HENACY has passed
static binary checks; console loading and stability still require hardware testing.

The existing password screen is a browser convenience gate, not secure access.

Run `npm test` to check selection, firmware gating, redirects and file integrity.
Run `npm run test:native` on Linux/x86-64 with a C compiler to test the native
System Information extension, including execution of the embedded code at a
relocated address against a simulated Mono/PUI interface. This checks pointer
binding and error handling; it does not constitute PS4 hardware verification.
Run `npm run build` to create the static Pages files in `dist/`.

`scripts/henacy-system-info.c`, its linker script, and `patch-system-info.py`
contain the display extension and guarded binary integration. The patcher accepts
only the previous HNCY menu-icon payload (SHA-256 starting `7163545ede8f`).
