#!/usr/bin/env python3
"""Host tests; these do not certify a PS4 hardware runtime."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="henacy-vsh-tests-") as folder:
    for name, files in (
        ("menu", ["tests/vsh-menu.c", "native/vsh/menu.c"]),
        ("bootstrap", ["tests/vsh-bootstrap.c"]),
        ("detour", ["tests/vsh-detour.c", "native/vsh/detour.c"]),
        ("runtime", ["tests/vsh-runtime.c", "native/vsh/menu.c", "native/vsh/detour.c"]),
    ):
        binary = str(Path(folder) / name)
        subprocess.run([os.environ.get("CC", "cc"), "-D_DEFAULT_SOURCE", "-std=c11", "-Os", "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter"] +
                       files + ["-o", binary], cwd=root, check=True)
        subprocess.run([binary], cwd=root, check=True)
