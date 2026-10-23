# HENACY v1 — PlayStation Store tile replacement (planned)

**Target:** PS4 home screen, firmware 13.52. The PlayStation Store tile should be replaced visually by the HNCY icon and **HENACY v1** label. Pressing X must invoke an actual HENACY action; merely changing the image is not a functional replacement.

## What exists today

HENACY currently inserts a `★ HNCY` item inside the PS4 **Settings** application using a ShellUI settings menu plugin. It does **not** currently replace the PlayStation Store home-screen tile or its launch action. The working icon in the repository is `assets/hen_settings_icon.png`.

## Safe implementation gates

1. Identify the Store tile entry and any custom HENACY launcher using a **read-only** export of `/system_data/priv/mms/app.db`. Run `python3 scripts/inspect-home-tiles.py app.db`. The inspector will not write to or create a database.
2. Confirm a concrete HENACY launch destination. A custom tile without a launch target would still start the Store, or fail.
3. Choose a reversible method. Prefer a validated runtime ShellUI/ShellCore hook over persistent console database changes; the existing firmware 13.52 browser exploit must remain usable.
4. Test on the target firmware using a separate build, with Store restoration and rescue instructions ready. Only after successful tests should the behavior be enabled by default.

**No unverified binary offsets or PS4 system files have been patched.** Do not alter `app.db` on the PS4 from an untested script. Editing the home-screen database has been reported by some PS4 users to cause WebKit slowdowns, which is especially undesirable when HENACY's exploit is launched from the browser.

This is development scaffolding, **not** an implemented Store tile override. The existing default HENACY payload and original HEN option remain unchanged.