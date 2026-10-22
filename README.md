# PS4 13.52 host

Select a payload using the links on the host page:

| Payload | Selection |
| --- | --- |
| HENACY v1 | Default |
| Original PS4 HEN 2.2.0 | `?original=1` |

HENACY v1 uses the **HNCY** PS4 settings-menu label and the red HNCY emblem.
The 128×128 RGBA menu image is included at `assets/hen_settings_icon.png` and
embedded in the menu plugin. Loading HENACY writes it to
`/data/hen/shellui_data/icons/hen_settings_icon.png`. Its payload is stored at
`loader/henacy.bin` (500,448 bytes), with SHA-256:

```
7163545ede8f6990539f73a22f1dc13f1ecd1f9019e7978c8d91a979c197e752
```

The host verifies payload size and SHA-256 before launching. HENACY has passed
static binary checks; console loading and stability still require hardware testing.

The existing password screen is a browser convenience gate, not secure access.

Run `npm test` to check selection, firmware gating, redirects and file integrity.
Run `npm run build` to create the static Pages files in `dist/`.
