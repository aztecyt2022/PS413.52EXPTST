#!/usr/bin/env python3
"""Build the PS4 native VSH PRX and a separate, firmware-pinned test payload.

Requires OpenOrbis v0.5.4 and Clang/LLD, or --cc /path/to/zig.
The existing HENACY and original HEN payloads are never overwritten.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BASE_SHA = "ea1235b846b08434123feec9b9c8e3d16cd9447ec31e0bd115df80589aa42352"
SELF = 0x29C80
TEXT = 0x2A450


def run(args):
    subprocess.run([str(a) for a in args], cwd=ROOT, check=True)


def sections(elf):
    if elf[:6] != b"\x7fELF\x02\x01" or struct.unpack_from("<H", elf, 18)[0] != 62:
        raise ValueError("Expected an x86-64 little-endian ELF")
    at = struct.unpack_from("<Q", elf, 40)[0]
    stride, count, names_index = struct.unpack_from("<HHH", elf, 58)
    entries = [struct.unpack_from("<IIQQQQIIQQ", elf, at + i * stride) for i in range(count)]
    names = entries[names_index]
    strings = elf[names[4]:names[4] + names[5]]
    return {strings[s[0]:strings.index(0, s[0])].decode(): s for s in entries}


def patch_bootstrap(base, extension):
    if len(base) != 500448 or hashlib.sha256(base).hexdigest() != BASE_SHA:
        raise ValueError("Unrecognized active HENACY payload")
    call = TEXT + 0x1D0F
    if base[call] != 0xE8:
        raise ValueError("Unrecognized native boot call")
    old_entry = 0x1D14 + struct.unpack_from("<i", base, call + 1)[0]
    if not 0x12040 <= old_entry < 0x12C80:
        raise ValueError("Unrecognized System Information extension")
    entry = struct.unpack_from("<Q", extension, 24)[0]
    output = bytearray(base)
    allowed = []
    for name, s in sections(extension).items():
        if s[1] in (4, 9):
            raise ValueError("Unresolved bootstrap relocation")
        if s[2] & 2 and s[5]:
            if name not in (".text", ".rodata"):
                raise ValueError("Unexpected bootstrap section: " + name)
            low, high = (0x13000, 0x13F80) if name == ".text" else (0xF900, 0x1081A)
            if not low <= s[3] < s[3] + s[5] <= high:
                raise ValueError("Bootstrap exceeds verified padding")
            start, end = TEXT + s[3], TEXT + s[3] + s[5]
            if any(base[start:end]):
                raise ValueError("Bootstrap would overwrite existing plugin data")
            output[start:end] = extension[s[4]:s[4] + s[5]]
            allowed.append((start, end))
    if not 0x13000 <= entry < 0x13F80:
        raise ValueError("Invalid bootstrap entry")
    struct.pack_into("<i", output, call + 1, entry - 0x1D14)
    allowed.append((call + 1, call + 5))
    # Verify dynamic symbols/relocations do not occupy the injected ranges.
    dyn = base[0x4A760:0x4C3E0]
    for plt, expected in ((0xABA0, b"wzvqT4UqKX8#B#B"), (0xAB10, b"LwG8g3niqwA#B#B")):
        slot = (plt - 0xA950) // 16
        _, relocation, _ = struct.unpack_from("<QQq", dyn, 0xFF8 + slot * 24)
        symbol = relocation >> 32
        name = struct.unpack_from("<I", dyn, 0x6F8 + symbol * 24)[0] + 0x18
        if dyn[name:dyn.index(0, name)] != expected:
            raise ValueError("Native loader import binding changed")
    virtual = [(lo - TEXT, hi - TEXT) for lo, hi in allowed[:-1]]
    for offset, count in ((0xFF8, 77), (0x1730, 10)):
        for i in range(count):
            address, relocation, addend = struct.unpack_from("<QQq", dyn, offset + i * 24)
            if any(lo <= address < hi or ((relocation & 0xFFFFFFFF) == 8 and lo <= addend < hi) for lo, hi in virtual):
                raise ValueError("Bootstrap collides with a native relocation")
    for i in range(96):
        _, _, _, section, address, size = struct.unpack_from("<IBBHQQ", dyn, 0x6F8 + i * 24)
        if section and any(address < hi and lo < address + max(1, size) for lo, hi in virtual):
            raise ValueError("Bootstrap collides with a native symbol")
    changed = [i for i, (a, b) in enumerate(zip(base, output)) if a != b]
    if not all(any(lo <= i < hi for lo, hi in allowed) for i in changed):
        raise ValueError("Unexpected payload modification")
    if output[0xC340:0x143C0] != base[0xC340:0x143C0]:
        raise ValueError("Firmware data changed")
    return bytes(output), old_entry, allowed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk", type=Path, default=os.environ.get("OO_PS4_TOOLCHAIN"))
    parser.add_argument("--cc", default="clang")
    parser.add_argument("--ld", default="ld.lld")
    parser.add_argument("--out", type=Path, default=ROOT / "builds/vsh-v1.2")
    args = parser.parse_args()
    if args.sdk is None:
        parser.error("Pass --sdk or set OO_PS4_TOOLCHAIN")
    sdk = args.sdk.resolve(); out = args.out.resolve()
    if out == (ROOT / "builds/vsh").resolve():
        parser.error("Refusing to overwrite the verified VSH 0.1 recovery build")
    out.mkdir(parents=True, exist_ok=True)
    os.environ["OO_PS4_TOOLCHAIN"] = str(sdk)
    if not (sdk / "bin/linux/create-fself").is_file():
        parser.error("OpenOrbis v0.5.4 SDK not found")
    base = (ROOT / "loader/henacy.bin").read_bytes()
    if hashlib.sha256(base).hexdigest() != BASE_SHA:
        raise ValueError("Active HENACY payload changed; review the bootstrap bindings before rebuilding")
    old_entry = 0x1D14 + struct.unpack_from("<i", base, TEXT + 0x1D10)[0]
    cc = [args.cc, "cc"] if Path(args.cc).name == "zig" else [args.cc]
    native_ld = [args.cc, "ld.lld"] if Path(args.cc).name == "zig" else [args.ld]
    target = ["-target", "x86_64-freebsd"]
    flags = target + ["-fPIC", "-fvisibility=hidden", "-ffreestanding", "-fno-stack-protector",
                      "-fno-unwind-tables", "-fno-asynchronous-unwind-tables", "-Os", "-Wall", "-Wextra", "-Werror",
                      "-isystem", str(sdk / "include")]
    with tempfile.TemporaryDirectory(prefix="henacy-vsh-build-") as temporary:
        folder = Path(temporary); objects = []
        for name in ("ps4", "menu", "detour"):
            obj = folder / (name + ".o"); objects.append(obj)
            run(cc + flags + ["-c", ROOT / ("native/vsh/" + name + ".c"), "-o", obj])
        module = folder / "module.o"; objects.append(module)
        run(cc + target + ["-c", ROOT / "native/vsh/module.S", "-o", module])
        elf = folder / "henacy_vsh.elf"
        run(native_ld + ["-m", "elf_x86_64", "-pie", "-T", sdk / "link.x", "-e", "_init",
            "--export-dynamic", "--eh-frame-hdr", "--build-id=none", "-o", elf] + objects +
            ["-L" + str(sdk / "lib"), "-lSceLibcInternal", "-lkernel", "-lmonosgen"])
        # Missing exports/undefined references are fatal at link time.
        symbols = subprocess.check_output(["readelf", "--dyn-syms", "--wide", str(elf)], text=True)
        if " HNCY_VshStart" not in symbols:
            raise ValueError("Native module entry point was not exported")
        run([sdk / "bin/linux/create-fself", "-in=" + str(elf), "-out=" + str(folder / "vsh.oelf"),
             "--lib=" + str(out / "henacy_vsh.prx"), "--paid", "0x3800000000000011"])
        linker = folder / "bootstrap.ld"
        linker.write_text(f"""ENTRY(HNCY_VshBoot)
Root_Domain = 0x202a0; App_Exe = 0x202a8;
HNCY_SystemInfoBoot = {old_entry:#x}; mono_get_image = 0x59f0;
sys_proc_rw = 0x3600; hde64_disasm = 0x2320;
sceKernelLoadStartModule = 0xaba0; sceKernelDlsym = 0xab10; printf = 0xa9d0;
SECTIONS {{
 .text 0x13000 : {{ *(.text .text.*) }}
 .rodata 0xf900 : {{ *(.rodata .rodata.*) }}
 /DISCARD/ : {{ *(.eh_frame*) *(.comment) *(.note*) }}
}}
ASSERT(ADDR(.text) + SIZEOF(.text) <= 0x13f80, "Bootstrap RX overflow")
ASSERT(ADDR(.rodata) + SIZEOF(.rodata) <= 0x1081a, "Bootstrap constants overflow")
""")
        # A freestanding GCC object uses the same x86-64 System V ABI here.
        run(["cc", "-Os", "-fPIE", "-fvisibility=hidden", "-ffreestanding", "-fno-builtin",
             "-fno-stack-protector", "-fno-unwind-tables", "-fno-asynchronous-unwind-tables",
             "-c", ROOT / "native/vsh/bootstrap.c", "-o", folder / "bootstrap.o"])
        run(["ld", "-T", linker, folder / "bootstrap.o", "-o", folder / "bootstrap.elf"])
        candidate, _, ranges = patch_bootstrap(base, (folder / "bootstrap.elf").read_bytes())
        (out / "henacy-vsh.bin").write_bytes(candidate)
        mono_size = struct.unpack_from("<Q", base, SELF + 16)[0]
        if mono_size != 141152 or SELF + mono_size != 0x4C3E0:
            raise ValueError("Embedded plugin_mono SELF size changed")
        (out / "plugin_mono_vsh.prx").write_bytes(candidate[SELF:SELF + mono_size])
        (out / "plugin_mono_original.prx").write_bytes(base[SELF:SELF + mono_size])
        manifest = {"version": "1.2.0", "platform": "PS4", "firmware": "13.52",
                    "akari_baseline": "1.2.2", "base_sha256": BASE_SHA,
                    "runtime_tested_on_ps4": False,
                    "payload_modified_ranges": [[hex(a), hex(b)] for a, b in ranges], "files": {}}
        for name in ("henacy_vsh.prx", "henacy-vsh.bin", "plugin_mono_vsh.prx", "plugin_mono_original.prx"):
            data = (out / name).read_bytes()
            manifest["files"][name] = {"size": len(data), "sha256": hashlib.sha256(data).hexdigest()}
        (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
        print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    main()
