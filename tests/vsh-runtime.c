#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static char config_path[256], config_temp[256], log_path[256];
#define CONFIG_PATH config_path
#define CONFIG_TEMP config_temp
#define LOG_PATH log_path
#define _init vsh_test_init
#define _fini vsh_test_fini
#include "../native/vsh/ps4.c"
#undef _init
#undef _fini

static uint64_t clock_us = 1000000;
static uint32_t firmware = 0x13520001;
uint64_t sceKernelGetProcessTime(void) { return clock_us += 16000; }
int sceKernelGetSystemSwVersion(VshFirmware *version) { version->version = firmware; return 0; }
const char *sceKernelGetFsSandboxRandomWord(void) { return "unit-test"; }
int sceKernelDlsym(int module, const char *name, void **out) { (void)module; (void)name; *out = NULL; return -1; }
int sceKernelGetModuleList(int *list, size_t size, size_t *count) { (void)list; (void)size; (void)count; return -1; }
int sceKernelGetModuleInfo(int module, VshModuleInfo *info) { (void)module; (void)info; return -1; }
#define NULL_METHOD(name, signature) void *name signature { return NULL; }
/* Only the failed-capability path is modelled: no fake successful PS4 UI test. */
NULL_METHOD(mono_class_from_name, (const void *a, const char *b, const char *c))
NULL_METHOD(mono_class_get_method_from_name, (const void *a, const char *b, int c))
NULL_METHOD(mono_class_get_property_from_name, (const void *a, const char *b))
NULL_METHOD(mono_property_get_get_method, (const void *a))
NULL_METHOD(mono_property_get_set_method, (const void *a))
NULL_METHOD(mono_aot_get_method, (const void *a, const void *b))
NULL_METHOD(mono_runtime_invoke, (const void *a, const void *b, const void **c, const void **d))
NULL_METHOD(mono_object_new, (const void *a, const void *b))
NULL_METHOD(mono_object_unbox, (void *a))
NULL_METHOD(mono_string_new, (const void *a, const char *b))
uint32_t mono_gchandle_new(void *a, int b) { (void)a; (void)b; return 0; }
NULL_METHOD(mono_gchandle_get_target, (uint32_t a))
void mono_gchandle_free(uint32_t handle) { (void)handle; }
static void *no_image(const char *path) { (void)path; return NULL; }
static int no_write(int pid, uintptr_t address, const void *source, size_t n, int write) {
    (void)pid; (void)address; (void)source; (void)n; (void)write;
    assert(!"Capability checks must not write process memory"); return -1;
}
static unsigned no_decode(const void *data, VshInstruction *instruction) {
    (void)data; (void)instruction; return 0;
}
static int disconnected(int handle, VshPadData *data) { (void)handle; (void)data; return -1; }
int main(void) {
    char directory[] = "/tmp/henacy-vsh-runtime-XXXXXX"; assert(mkdtemp(directory));
    snprintf(config_path, sizeof(config_path), "%s/config", directory);
    snprintf(config_temp, sizeof(config_temp), "%s/config.tmp", directory);
    snprintf(log_path, sizeof(log_path), "%s/runtime.log", directory);
    HncyVshHost api = {sizeof(api), HNCY_VSH_ABI, (void *)1, (void *)2, no_image, no_write, no_decode};
    assert(HNCY_VshStart(NULL) == -1);
    firmware = 0x13500001; assert(HNCY_VshStart(&api) == -2);
    firmware = 0x13520001; assert(HNCY_VshStart(&api) == -3);
    assert(!started && !atomic_load(&home_ready) && !read_hook.installed && !state_hook.installed && !update_hook.installed);
    vsh_init(&menu); atomic_store(&home_ready, true); atomic_store(&last_frame, clock_us);
    VshPadData pad = {.buttons = VSH_CHORD, .connected = 1, .timestamp = 1};
    pad.orientation[0] = 0.25f; pad.count = 9;
    filter_pad(10, &pad); assert(menu.open && menu.draining);
    assert(pad.buttons == 0 && pad.left[0] == 128 && pad.right[1] == 128);
    assert(pad.connected == 1 && pad.timestamp == 1 && pad.orientation[0] == 0.25f && pad.count == 9);
    filter_pad(10, &pad); assert(menu.draining); /* A nested ReadState cannot clear the opening latch. */
    pad.timestamp = 2; pad.buttons = 0; filter_pad(10, &pad); assert(!menu.draining);
    menu.page = VSH_OVERLAY;
    pad.timestamp = 3; pad.buttons = VSH_CROSS; filter_pad(10, &pad); assert(menu.config.stats == 1);
    filter_pad(10, &pad); assert(menu.previous == VSH_CROSS && menu.config.stats == 1);
    pad.timestamp = 4; pad.buttons = VSH_CROSS; filter_pad(10, &pad); assert(menu.config.stats == 1);
    state_hook.trampoline = (void *)disconnected;
    assert(pad_state(10, &pad) == -1); assert(!menu.open && menu.owner == -1);
    /* A stalled UI must stop capture even if the pad sample is duplicated. */
    vsh_init(&menu); menu.open = true; menu.owner = 10;
    last_pad_timestamp = pad.timestamp; last_pad_handle = 10; last_pad_consumed = true;
    clock_us += 600000; filter_pad(10, &pad); assert(!menu.open);
    pad.buttons = 0; filter_pad(10, &pad);
    pad.buttons = VSH_CROSS; filter_pad(10, &pad); assert(pad.buttons == VSH_CROSS);
    vsh_init(&menu); menu.config.theme = 2; menu.config.opacity = 95;
    assert(save_config(&menu.config)); assert(access(config_temp, F_OK) != 0);
    vsh_init(&menu); read_config(); assert(menu.config.theme == 2 && menu.config.opacity == 95);
    FILE *f = fopen(config_path, "w"); assert(f);
    fputs("HENACY_VSH_1\nstats=0\nside=0\ntheme=256\nopacity=95\nfahrenheit=0\n", f); fclose(f);
    vsh_init(&menu); read_config(); assert(menu.config.theme == 3 && menu.config.opacity == 90);
    menu.config.opacity = 69; assert(!save_config(&menu.config));
    remove(config_path); remove(log_path); assert(rmdir(directory) == 0);
    puts("PASS: unsupported firmware/API fail without hooks; real PS4 pad layout filtering, nested reads, disconnect handling and config round-trip/rejection.");
    return 0;
}
