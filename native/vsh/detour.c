#include "detour.h"
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static void jump(uint8_t *out, uintptr_t destination) {
    const uint8_t prefix[] = {0xff, 0x25, 0, 0, 0, 0};
    memcpy(out, prefix, 6); memcpy(out + 6, &destination, 8);
}
bool vsh_hook_prepare(VshHook *h, void *target, const HncyVshHost *host) {
    if (!target || h->trampoline || !host || !host->decode) return false;
    const uint8_t *code = target;
    size_t length = 0;
    while (length < 14) {
        VshInstruction insn = {0};
        unsigned n = host->decode(code + length, &insn);
        if (!n || n > 15 || length + n > sizeof(h->original) ||
            (insn.flags & (0x1000u | 0x200u)) ||
            ((insn.flags & 1u) && insn.modrm_mod == 0 && insn.modrm_rm == 5) ||
            insn.opcode == 0xc2 || insn.opcode == 0xc3 || insn.opcode == 0xca || insn.opcode == 0xcb ||
            (insn.opcode == 0xff && insn.modrm_reg >= 2 && insn.modrm_reg <= 5)) return false;
        length += n;
    }
    uint8_t *trampoline = mmap(NULL, 0x4000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (trampoline == MAP_FAILED) return false;
    memcpy(h->original, code, length); memcpy(trampoline, code, length);
    jump(trampoline + length, (uintptr_t)code + length);
    if (mprotect(trampoline, 0x4000, PROT_READ | PROT_EXEC) != 0) {
        munmap(trampoline, 0x4000); return false;
    }
    h->target = target; h->trampoline = trampoline; h->length = length;
    return true;
}
bool vsh_hook_install(VshHook *h, void *replacement, const HncyVshHost *host) {
    if (!h->trampoline || h->installed || memcmp(h->target, h->original, h->length)) return false;
    uint8_t patch[14]; jump(patch, (uintptr_t)replacement);
    if (host->proc_rw(getpid(), (uintptr_t)h->target, patch, sizeof(patch), 1) == 0 &&
        memcmp(h->target, patch, sizeof(patch)) == 0) { h->installed = true; return true; }
    /* Retain the trampoline even if restoration fails: never unload live code. */
    h->installed = true;
    vsh_hook_restore(h, host);
    return false;
}
bool vsh_hook_restore(VshHook *h, const HncyVshHost *host) {
    if (!h->installed) return true;
    if (host->proc_rw(getpid(), (uintptr_t)h->target, h->original, 14, 1) != 0 ||
        memcmp(h->target, h->original, 14)) return false;
    h->installed = false;
    return true;
}
