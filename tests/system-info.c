/* Host model of Mono/PUI plus an executable x86-64 setter/trampoline.
 * This exercises the extension's real detour, not a JavaScript byte mock.
 * It does not replace testing ShellUI on a PS4.
 */
#define _GNU_SOURCE
#define HNCY_TEST
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "../scripts/henacy-system-info.c"

void *Root_Domain = (void *)1, *App_Exe = (void *)2;
unsigned char hncy_text_trampoline[64] __attribute__((aligned(4096)));
enum { INSTANCE = 1, DISPLAY_GET, DISPLAY_SET, CTOR, LOAD, APPEND, FIND, REMOVE,
       NAME, WIDTH, HEIGHT, X, Y, CLIP, TEXT };
static int methods[TEXT + 1], properties[TEXT + 1];
static int app_class, widget_class, label_class, image_class;
static HNCYString *display;
static uint8_t *native_setter;
static uint8_t *plugin_image;
static unsigned checks, images, loads, native_calls;
static bool no_ui, set_error, load_error, write_error;
typedef struct Image { void *name; float width, height; void *owner; } Image;
typedef struct Label { void *text; Image *image; } Label;

static HNCYString *string(const char *utf8) {
    size_t n = strlen(utf8);
    HNCYString *s = calloc(1, sizeof(*s) + (n + 1) * 2);
    assert(s);
    for (size_t i = 0; i < n;) {
        unsigned v = (uint8_t)utf8[i++];
        if (v >= 0xe0) { v = ((v & 15) << 12) | (((uint8_t)utf8[i] & 63) << 6) | ((uint8_t)utf8[i + 1] & 63); i += 2; }
        s->characters[s->length++] = (uint16_t)v;
    }
    return s;
}
void *mono_string_new(const void *domain, const char *value) { assert(domain); return string(value); }
char *mono_string_to_utf8(const void *value) {
    const HNCYString *s = value;
    char *out = calloc(s->length * 3 + 1, 1); assert(out);
    size_t j = 0;
    for (size_t i = 0; i < s->length; ++i) {
        unsigned v = s->characters[i];
        if (v < 128) out[j++] = (char)v;
        else { out[j++] = (char)(0xe0 | (v >> 12)); out[j++] = (char)(0x80 | ((v >> 6) & 63)); out[j++] = (char)(0x80 | (v & 63)); }
    }
    return out;
}
void mono_free(const void *p) { free((void *)p); }
void MainThreadCheck(void) { ++checks; }
const char *sceKernelGetFsSandboxRandomWord(void) { return "test-sandbox"; }
void *mono_get_image(const char *path) { assert(strstr(path, "/test-sandbox/common/lib/")); return no_ui ? NULL : (void *)3; }
void *mono_class_from_name(const void *image, const char *ns, const char *name) {
    assert(image);
    if (!strcmp(ns, "Sce.Vsh.ShellUI.AppSystem") && !strcmp(name, "SystemSoftwareVersionInfo")) return &app_class;
    assert(!strcmp(ns, "Sce.PlayStation.PUI.UI2"));
    if (!strcmp(name, "Widget")) return &widget_class;
    if (!strcmp(name, "Label")) return &label_class;
    if (!strcmp(name, "ImageBox")) return &image_class;
    return NULL;
}
void *mono_class_get_property_from_name(const void *klass, const char *name) {
    const char *names[] = {[INSTANCE]="Instance",[DISPLAY_GET]="DisplayVersion",[NAME]="Name",[WIDTH]="Width",[HEIGHT]="Height",[X]="X",[Y]="Y",[CLIP]="ClipChildren",[TEXT]="Text"};
    (void)klass;
    for (int i = 1; i <= TEXT; ++i) if (names[i] && !strcmp(name, names[i])) return &properties[i];
    return NULL;
}
void *mono_property_get_get_method(const void *property) {
    ptrdiff_t i = (const int *)property - properties;
    return i == INSTANCE || i == DISPLAY_GET ? &methods[i] : NULL;
}
void *mono_property_get_set_method(const void *property) {
    ptrdiff_t i = (const int *)property - properties;
    return &methods[i == DISPLAY_GET ? DISPLAY_SET : i];
}
void *mono_class_get_method_from_name(const void *klass, const char *name, int count) {
    (void)klass;
    if (!strcmp(name, ".ctor") && count == 0) return &methods[CTOR];
    if (!strcmp(name, "LoadAsync") && count == 1) return &methods[LOAD];
    if (!strcmp(name, "AppendChild") && count == 1) return &methods[APPEND];
    if (!strcmp(name, "FindChildByName") && count == 1) return &methods[FIND];
    if (!strcmp(name, "RemoveFromParent") && count == 0) return &methods[REMOVE];
    return NULL;
}
void *mono_aot_get_method(const void *domain, const void *method) { assert(domain && method == &methods[TEXT]); return native_setter; }
void *mono_object_new(const void *domain, const void *klass) { assert(domain && klass == &image_class); ++images; return calloc(1, sizeof(Image)); }
void *mono_runtime_invoke(const void *method, const void *object, const void **args, const void **exception) {
    ptrdiff_t id = (const int *)method - methods;
    switch (id) {
    case INSTANCE: return &app_class;
    case DISPLAY_GET: return display;
    case DISPLAY_SET:
        if (set_error) { *exception = (void *)1; set_error = false; }
        else display = (void *)args[0];
        break;
    case CTOR: break;
    case FIND: return ((Label *)object)->image;
    case NAME: ((Image *)object)->name = (void *)args[0]; break;
    case WIDTH: ((Image *)object)->width = *(const float *)args[0]; break;
    case HEIGHT: ((Image *)object)->height = *(const float *)args[0]; break;
    case X: case Y: case CLIP: break;
    case LOAD: {
        char *uri = mono_string_to_utf8(args[0]); assert(!strcmp(uri, logo_uri)); free(uri);
        ++loads; if (load_error) *exception = (void *)1;
        break;
    }
    case APPEND: ((Label *)object)->image = (void *)args[0]; ((Image *)args[0])->owner = (void *)object; break;
    case REMOVE: {
        Image *image = (void *)object;
        if (image->owner) ((Label *)image->owner)->image = NULL;
        image->owner = NULL;
        break;
    }
    default: assert(!"unexpected managed call");
    }
    return NULL;
}
int sys_proc_rw(int pid, uintptr_t target, const void *source, size_t size, int write) {
    assert(pid == getpid() && write == 1);
    if (write_error) return -1;
    memcpy((void *)target, source, size);
    return 0;
}
static void native_text(void *object, void *text) { ++native_calls; ((Label *)object)->text = text; }
static void reset(void) {
    memset(&info, 0, sizeof(info));
    no_ui = set_error = load_error = write_error = false;
    images = loads = native_calls = 0;
    Root_Domain = (void *)1; App_Exe = (void *)2;
    display = string("13.52");
    const uint8_t prefix[] = {0x55,0x48,0x89,0xe5,0x48,0x83,0xec,0x20,0x48,0x89,0x7d,0xf8,0x48,0x89,0x75,0xf0,0xc9};
    memcpy(native_setter, prefix, sizeof(prefix));
    jump(native_setter + sizeof(prefix), (uintptr_t)native_text);
}
static void text(Label *label, void *value) { ((void (*)(void *, void *))native_setter)(label, value); }
static void expect(void *value, const char *wanted) { char *v = mono_string_to_utf8(value); assert(!strcmp(v, wanted)); free(v); }

/* Execute the payload's existing decoder with its own data table. */
unsigned hde64_disasm(const void *code, HNCYInsn *instruction) {
    return ((unsigned (*)(const void *, HNCYInsn *))(plugin_image + 0x2320))(code, instruction);
}
static uint16_t u16(const uint8_t *p) { uint16_t n; memcpy(&n,p,2); return n; }
static uint32_t u32(const uint8_t *p) { uint32_t n; memcpy(&n,p,4); return n; }
static uint64_t u64(const uint8_t *p) { uint64_t n; memcpy(&n,p,8); return n; }
static void load_plugin(const char *path) {
    FILE *file = fopen(path, "rb"); assert(file);
    assert(fseek(file,0,SEEK_END) == 0); long size = ftell(file); rewind(file);
    uint8_t *payload = malloc((size_t)size); assert(payload);
    assert(fread(payload,1,(size_t)size,file) == (size_t)size); fclose(file);
    size_t base = 0x29c80;
    assert(size >= 500448 && u32(payload+base) == 0x1d3d154f);
    unsigned count = u16(payload+base+24);
    const uint8_t *elf = payload+base+32+count*32;
    assert(u32(elf) == 0x464c457f);
    plugin_image = mmap(NULL,0x24000,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    assert(plugin_image != MAP_FAILED);
    for (unsigned i=0;i<count;++i) {
        const uint8_t *entry = payload+base+32+i*32;
        uint64_t flags = u64(entry);
        if (!(flags & 0x800)) continue;
        const uint8_t *ph = elf+u64(elf+32)+((flags>>20)&0xfff)*u16(elf+54);
        if (u32(ph) != 1 && u32(ph) != 0x61000010) continue;
        uint64_t virtual = u64(ph+16), raw = base+u64(entry+8), length = u64(entry+16);
        assert(virtual+length <= 0x24000 && raw+length <= (uint64_t)size);
        memcpy(plugin_image+virtual,payload+raw,length);
    }
    free(payload);
    jump(plugin_image+0xa950,(uintptr_t)memset);
}
static void payload_functions(void) {
    struct Binding { uintptr_t at; uintptr_t function; } bindings[] = {
        {0x1d30,(uintptr_t)MainThreadCheck},{0x59f0,(uintptr_t)mono_get_image},
        {0xabe0,(uintptr_t)mono_class_from_name},{0xabf0,(uintptr_t)mono_class_get_property_from_name},
        {0xac00,(uintptr_t)mono_property_get_get_method},{0xacd0,(uintptr_t)mono_property_get_set_method},
        {0xac20,(uintptr_t)mono_class_get_method_from_name},{0xac30,(uintptr_t)mono_aot_get_method},
        {0xac10,(uintptr_t)mono_runtime_invoke},{0xac90,(uintptr_t)mono_object_new},
        {0xac40,(uintptr_t)mono_string_new},{0xacb0,(uintptr_t)mono_string_to_utf8},
        {0xacc0,(uintptr_t)mono_free},{0xaaa0,(uintptr_t)sceKernelGetFsSandboxRandomWord},
        {0xaab0,(uintptr_t)getpid},{0x3600,(uintptr_t)sys_proc_rw},
        {0xa9c0,(uintptr_t)snprintf},{0xa9d0,(uintptr_t)printf}
    };
    for (size_t i=0;i<sizeof(bindings)/sizeof(bindings[0]);++i) jump(plugin_image+bindings[i].at,bindings[i].function);
    memcpy(plugin_image+0x202a0,&Root_Domain,sizeof(Root_Domain));
    memcpy(plugin_image+0x202a8,&App_Exe,sizeof(App_Exe));
}

int main(int argc, char **argv) {
    const bool candidate = argc > 1;
    const char *embedded_brand = candidate ? "13.52 | DEXCORE" : "13.52 | HENACY v1";
    const char *embedded_padded = candidate ?
        "\xe2\x80\x83\xe2\x80\x83\xe2\x80\x83" "13.52 | DEXCORE" :
        "\xe2\x80\x83\xe2\x80\x83\xe2\x80\x83" "13.52 | HENACY v1";
    load_plugin(candidate ? argv[1] : "loader/henacy.bin");
    native_setter = mmap(NULL, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(native_setter != MAP_FAILED);
    assert(mprotect(hncy_text_trampoline, 4096, PROT_READ | PROT_WRITE | PROT_EXEC) == 0);
    reset(); Root_Domain = NULL; HNCY_SystemInfoBoot(); assert(checks == 1 && !info.image_ready);
    reset(); HNCY_SystemInfoBoot(); expect(display, "13.52 | HENACY v1"); assert(info.image_ready);
    Label label = {0}, other = {0};
    void *ordinary = string("Network"); text(&other, ordinary); assert(other.text == ordinary && !images && native_calls == 1);
    text(&label, display); assert(label.image && images == 1 && loads == 1);
    assert(label.image->width == 64 && label.image->height == 64);
    expect(label.image->name, logo_name); expect(label.text, "\xe2\x80\x83\xe2\x80\x83\xe2\x80\x83" "13.52 | HENACY v1");
    text(&label, display); assert(images == 1 && loads == 1);
    text(&label, ordinary); assert(!label.image && label.text == ordinary);
    load_error = true; text(&label, display); assert(!label.image); expect(label.text, "13.52 | HENACY v1");
    load_error = false; text(&label, display); assert(label.image);
    assert(apply_version(App_Exe)); assert(info.image_ready); expect(display, "13.52 | HENACY v1");
    reset(); no_ui = true; HNCY_SystemInfoBoot(); assert(!info.image_ready); expect(display, "13.52 | HENACY v1");
    reset(); write_error = true; HNCY_SystemInfoBoot(); assert(!info.image_ready); expect(display, "13.52 | HENACY v1");
    reset(); set_error = true; HNCY_SystemInfoBoot(); expect(display, "13.52");
    reset(); memset(native_setter, 0x90, 64); native_setter[0] = 0xe8; assert(!relocatable_prologue(native_setter));
    reset(); native_setter[0] = 0xc3; assert(!relocatable_prologue(native_setter));
    const uint8_t rip[] = {0x48,0x8b,0x05,0,0,0,0}; memcpy(native_setter, rip, sizeof(rip)); assert(!relocatable_prologue(native_setter));
    puts("PASS: native detour execution, original-label pass-through, logo creation/reuse/removal, image failure fallback, version preservation/idempotence/rollback, missing UI, write failure, unsafe prologue rejection.");
    /* Run the actual embedded extension at its relocated runtime address. */
    reset(); payload_functions();
    assert(plugin_image[0x1d0f] == 0xe8);
    int32_t displacement; memcpy(&displacement,plugin_image+0x1d10,4);
    uintptr_t entry = 0x1d14+(intptr_t)displacement;
    assert(entry >= 0x12040 && entry < 0x13f80);
    ((void (*)(void))(plugin_image+entry))();
    expect(display, embedded_brand);
    Label actual_label = {0}; text(&actual_label,display);
    assert(actual_label.image && actual_label.image->width == 64);
    expect(actual_label.text, embedded_padded);
    text(&actual_label,ordinary); assert(!actual_label.image);
    puts("PASS: the embedded payload code executes at a relocated base, resolves existing Mono calls, displays the expected brand with the existing HNCY icon, and releases the image when the label changes.");
    return 0;
}
