/* Execute the actual injected x86-64 bootstrap with modelled PS4 imports. */
#define main existing_system_info_main
#include "system-info.c"
#undef main
#include "../native/vsh/abi.h"
static int load_result, start_calls, find_calls;
static bool find_error;
static int model_start(const HncyVshHost *api) {
    ++start_calls;
    assert(api->size == sizeof(*api) && api->abi == 1);
    assert(api->domain == Root_Domain && api->app == App_Exe);
    assert((uintptr_t)api->get_image == (uintptr_t)plugin_image + 0x59f0);
    assert((uintptr_t)api->proc_rw == (uintptr_t)plugin_image + 0x3600);
    assert((uintptr_t)api->decode == (uintptr_t)plugin_image + 0x2320);
    VshInstruction instruction = {0};
    assert(api->decode("\x90", &instruction) == 1);
    return 0;
}
static int model_load(const char *path, size_t size, const void *data, unsigned flags, void *a, void *b) {
    assert(!strcmp(path, "/data/hen/henacy_vsh.prx"));
    assert(size == 0 && !data && flags == 0 && !a && !b);
    return load_result;
}
static int model_dlsym(int module, const char *symbol, void **target) {
    assert(module == 42 && !strcmp(symbol, "HNCY_VshStart")); ++find_calls;
    if (find_error) return -1;
    *target = (void *)model_start; return 0;
}
int main(void) {
    native_setter = mmap(NULL, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(native_setter != MAP_FAILED);
    assert(mprotect(hncy_text_trampoline, 4096, PROT_READ | PROT_WRITE | PROT_EXEC) == 0);
    for (int test = 0; test < 3; ++test) {
        load_plugin("builds/vsh/henacy-vsh.bin"); reset(); payload_functions();
        load_result = test == 0 ? -1 : 42; find_error = test == 1;
        start_calls = find_calls = 0;
        jump(plugin_image + 0xaba0, (uintptr_t)model_load);
        jump(plugin_image + 0xab10, (uintptr_t)model_dlsym);
        uint8_t *callsite = plugin_image + 0x1d0f;
        int32_t relative; memcpy(&relative, callsite + 1, 4);
        void (*boot)(void) = (void (*)(void))(callsite + 5 + relative);
        boot();
        expect(display, "13.52 | HENACY v1");
        assert(find_calls == (test != 0)); assert(start_calls == (test == 2));
        munmap(plugin_image, 0x24000);
    }
    puts("PASS: actual VSH bootstrap preserves System Information, handles a missing PRX/export, passes the exact native host ABI, and starts the module.");
    return 0;
}
