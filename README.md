# PS4 13.52 host

Select a payload using the links on the host page:

| Payload | Selection |
| --- | --- |
| HENACY v1 | Default |
| Original PS4 HEN 2.2.0 | `?original=1` |

HENACY v1 uses the **HNCY** PS4 settings-menu label. Its payload is stored at
`loader/henacy.bin` (500,448 bytes), with SHA-256:

```
869664c304ad44fd2ba81da2e3e5d9a24a17175698bfe4182ac35c6db4df86d6
```

The host verifies payload size and SHA-256 before launching. HENACY has passed
static binary checks; console loading and stability still require hardware testing.

The existing password screen is a browser convenience gate, not secure access.

Run `npm test` to check selection, firmware gating, redirects and file integrity.
Run `npm run build` to create the static Pages files in `dist/`.
