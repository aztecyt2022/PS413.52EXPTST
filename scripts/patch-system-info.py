#!/usr/bin/env python3
"""Insert the display extension into one explicitly identified HENACY image.

Usage: patch-system-info.py BASE_PAYLOAD LINKED_EXTENSION_ELF OUTPUT_PAYLOAD
Compile henacy-system-info.c with -Os -fPIE -fvisibility=hidden -ffreestanding
-fno-builtin -fno-stack-protector -fno-unwind-tables
-fno-asynchronous-unwind-tables -fno-jump-tables; link with henacy-system-info.ld.
Only plugin_mono padding, its boot call, and its BSS size may change.
"""
import hashlib
import json
from pathlib import Path
import struct
import sys

BASE_SHA256 = "7163545ede8f6990539f73a22f1dc13f1ecd1f9019e7978c8d91a979c197e752"
SELF = 0x29C80
TEXT = 0x2A450
FIRMWARE = slice(0xC340, 0x143C0)


def main():
    if len(sys.argv) != 4:
        raise SystemExit(__doc__)
    original = Path(sys.argv[1]).read_bytes()
    if len(original) != 500448 or hashlib.sha256(original).hexdigest() != BASE_SHA256:
        raise SystemExit("Refusing a payload other than the verified HENACY menu-icon build.")
    extension = Path(sys.argv[2]).read_bytes()
    assert extension[:6] == b"\x7fELF\x02\x01"
    entry, = struct.unpack_from("<Q", extension, 24)
    shoff, = struct.unpack_from("<Q", extension, 40)
    stride, count, names_index = struct.unpack_from("<HHH", extension, 58)
    sections = [struct.unpack_from("<IIQQQQIIQQ", extension, shoff + i * stride) for i in range(count)]
    names = sections[names_index]
    strings = extension[names[4]:names[4] + names[5]]
    named = {}
    for section in sections:
        name = strings[section[0]:strings.index(0, section[0])].decode()
        named[name] = section
        assert section[1] not in (4, 9), "The linked extension must not contain unresolved relocations."
    text, rodata, bss = (named[n] for n in (".text", ".rodata", ".bss"))
    assert text[3] == 0x12040 and text[3] + text[5] <= 0x13F80
    assert rodata[3] == 0xF620 and rodata[3] + rodata[5] <= 0x1081A
    assert bss[3] == 0x22800 and bss[3] + bss[5] <= 0x22C00
    assert text[3] <= entry < text[3] + text[5]
    payload = bytearray(original)
    allowed = []
    for section in (text, rodata):
        start = TEXT + section[3]
        end = start + section[5]
        assert not any(original[start:end]), "Extension would overwrite existing plugin data."
        payload[start:end] = extension[section[4]:section[4] + section[5]]
        allowed.append((start, end))
    call = TEXT + 0x1D0F
    assert payload[call:call + 5] == bytes.fromhex("e81c000000")
    struct.pack_into("<i", payload, call + 1, entry - 0x1D14)
    allowed.append((call + 1, call + 5))
    elf = SELF + 32 + struct.unpack_from("<H", payload, SELF + 24)[0] * 32
    phoff, = struct.unpack_from("<Q", payload, elf + 32)
    phstride, = struct.unpack_from("<H", payload, elf + 54)
    phdata = elf + phoff + phstride * 2
    assert struct.unpack_from("<Q", payload, phdata + 16)[0] == 0x20000
    assert struct.unpack_from("<Q", payload, phdata + 40)[0] == 0x2710
    # Both old and new BSS occupy the same 16 KiB memory allocation.
    assert (0x2710 + 0x3FFF) // 0x4000 == (0x2C00 + 0x3FFF) // 0x4000
    struct.pack_into("<Q", payload, phdata + 40, 0x2C00)
    allowed.append((phdata + 40, phdata + 48))
    assert payload[FIRMWARE] == original[FIRMWARE]
    assert payload[0x424C0:0x464AC] == original[0x424C0:0x464AC], "Menu artwork must stay identical."
    # Resolve every existing relocation and symbol range before accepting space.
    dynamic = original[0x4A760:0x4C3E0]
    reserved = [(text[3], text[3] + text[5]), (rodata[3], rodata[3] + rodata[5]), (bss[3], bss[3] + bss[5]), (0x13F80, 0x13FC0)]
    for offset, number in ((0xFF8, 77), (0x1730, 10)):
        for i in range(number):
            address, relocation, addend = struct.unpack_from("<QQq", dynamic, offset + i * 24)
            assert all(not (lo <= address < hi) for lo, hi in reserved)
            if (relocation & 0xFFFFFFFF) == 8:
                assert all(not (lo <= addend < hi) for lo, hi in reserved)
    for i in range(96):
        name, info, other, section, address, size = struct.unpack_from("<IBBHQQ", dynamic, 0x6F8 + i * 24)
        if section:
            assert all(not (address < hi and lo < address + max(1, size)) for lo, hi in reserved)
    changed = [i for i, (a, b) in enumerate(zip(original, payload)) if a != b]
    assert all(any(lo <= i < hi for lo, hi in allowed) for i in changed)
    output = Path(sys.argv[3])
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(payload)
    print(json.dumps({"size": len(payload), "sha256": hashlib.sha256(payload).hexdigest(),
                      "entry": hex(entry), "text_size": text[5], "constants_size": rodata[5],
                      "state_size": bss[5], "changed_bytes": len(changed),
                      "firmware_data_unchanged": True, "menu_icon_unchanged": True,
                      "runtime_tested_on_ps4": False}, indent=2))


if __name__ == "__main__":
    main()
