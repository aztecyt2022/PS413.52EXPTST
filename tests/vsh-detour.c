#define main existing_system_info_main
#include "system-info.c"
#undef main
#include "../native/vsh/detour.h"
static int replacement(void) { return 99; }
static unsigned decode(const void *bytes, VshInstruction *instruction) {
    return ((unsigned (*)(const void *, VshInstruction *))(plugin_image + 0x2320))(bytes, instruction);
}
int main(void) {
    load_plugin("loader/henacy.bin");
    HncyVshHost api = {.proc_rw = sys_proc_rw, .decode = decode};
    uint8_t *code = mmap(NULL, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(code != MAP_FAILED);
    memset(code, 0x90, 64);
    const uint8_t result[] = {0xb8, 7, 0, 0, 0, 0xc3};
    memcpy(code + 14, result, sizeof(result));
    VshHook hook = {0};
    assert(((int (*)(void))code)() == 7);
    assert(vsh_hook_prepare(&hook, code, &api));
    assert(((int (*)(void))hook.trampoline)() == 7);
    assert(vsh_hook_install(&hook, replacement, &api));
    assert(((int (*)(void))code)() == 99);
    assert(((int (*)(void))hook.trampoline)() == 7);
    assert(vsh_hook_restore(&hook, &api));
    assert(((int (*)(void))code)() == 7);
    write_error = true;
    assert(!vsh_hook_install(&hook, replacement, &api));
    assert(((int (*)(void))code)() == 7);
    write_error = false; assert(vsh_hook_restore(&hook, &api));
    const uint8_t unsafe[][8] = {
        {0xe8, 0, 0, 0, 0}, {0xe9, 0, 0, 0, 0},
        {0x48, 0x8b, 0x05, 0, 0, 0, 0}, {0xc3}, {0xff, 0xe0}, {0x75, 0x05}
    };
    for (unsigned i = 0; i < sizeof(unsafe) / sizeof(unsafe[0]); ++i) {
        memset(code, 0x90, 64); memcpy(code, unsafe[i], sizeof(unsafe[i]));
        VshHook refused = {0}; assert(!vsh_hook_prepare(&refused, code, &api));
        assert(!refused.trampoline && !refused.installed);
    }
    munmap(hook.trampoline, 0x4000); munmap(code, 4096); munmap(plugin_image, 0x24000);
    puts("PASS: real x86-64 detour/trampoline execution, restoration, failed writes and rejection of relative/indirect control flow.");
    return 0;
}
