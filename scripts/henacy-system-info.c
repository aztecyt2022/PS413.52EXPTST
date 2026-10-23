/* HENACY v1: display-only branding in the native System Information row.
 *
 * SystemSoftwareVersionInfo.DisplayVersion and PUI Label/Widget APIs are
 * documented in OSM-Made/PS4-Mono and OSM-Made/Orbis-Toolbox. ImageBox and
 * LoadAsync are also used by AetherPS/Fusion-Plugins. No Sony firmware code,
 * page identifier, or firmware memory address is embedded here.
 *
 * The linker binds this extension to the existing plugin_mono functions in
 * the checked HENACY payload. All managed calls catch exceptions. The optional
 * image hook accepts only a relocatable, decoded native method prologue.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef HNCY_TEST
#define HIDDEN
#else
#define HIDDEN __attribute__((visibility("hidden")))
#endif
extern HIDDEN void *Root_Domain, *App_Exe;
extern HIDDEN void MainThreadCheck(void);
extern HIDDEN void *mono_get_image(const char *);
extern HIDDEN void *mono_class_from_name(const void *, const char *, const char *);
extern HIDDEN void *mono_class_get_property_from_name(const void *, const char *);
extern HIDDEN void *mono_property_get_get_method(const void *);
extern HIDDEN void *mono_property_get_set_method(const void *);
extern HIDDEN void *mono_class_get_method_from_name(const void *, const char *, int);
extern HIDDEN void *mono_aot_get_method(const void *, const void *);
extern HIDDEN void *mono_runtime_invoke(const void *, const void *, const void **, const void **);
extern HIDDEN void *mono_object_new(const void *, const void *);
extern HIDDEN void *mono_string_new(const void *, const char *);
extern HIDDEN char *mono_string_to_utf8(const void *);
extern HIDDEN void mono_free(const void *);
extern HIDDEN const char *sceKernelGetFsSandboxRandomWord(void);
extern HIDDEN int getpid(void);
extern HIDDEN int sys_proc_rw(int, uintptr_t, const void *, size_t, int);
extern HIDDEN int snprintf(char *, size_t, const char *, ...);
extern HIDDEN int printf(const char *, ...);
extern HIDDEN unsigned char hncy_text_trampoline[64];

/* ABI of the instruction decoder already present in plugin_mono (HDE64). */
typedef struct __attribute__((packed)) {
    uint8_t len, p_rep, p_lock, p_seg, p_66, p_67, rex, rex_w, rex_r, rex_x,
            rex_b, opcode, opcode2, modrm, modrm_mod, modrm_reg, modrm_rm,
            sib, sib_scale, sib_index, sib_base;
    uint64_t immediate;
    uint32_t displacement, flags;
} HNCYInsn;
_Static_assert(sizeof(HNCYInsn) == 37, "HDE64 decoder ABI must stay packed");
extern HIDDEN unsigned hde64_disasm(const void *, HNCYInsn *);

typedef struct {
    void *headers[2];
    uint32_t length;
    uint16_t characters[];
} HNCYString;

static struct {
    char version[160];
    char padded[176];
    void *image_class, *ctor, *load, *append, *find, *remove;
    void *name, *width, *height, *x, *y, *clip;
    void (*text_original)(void *, void *);
    void *last_label; /* Identity only; never dereferenced outside a live callback. */
    int load_arguments;
    bool image_ready;
} info;

static const char suffix[] = " | HENACY v1";
static const char logo_name[] = "hncy_system_information_logo";
static const char logo_uri[] = "file:///user/data/hen/shellui_data/icons/hen_settings_icon.png";

static size_t length(const char *s) { size_t n = 0; while (s[n]) ++n; return n; }
static bool equal(const char *a, const char *b) {
    while (*a && *a == *b) { ++a; ++b; }
    return *a == *b;
}
static bool same_bytes(const void *aa, const void *bb, size_t n) {
    const uint8_t *a = aa, *b = bb;
    for (size_t i = 0; i < n; ++i) if (a[i] != b[i]) return false;
    return true;
}
static void copy_bytes(void *aa, const void *bb, size_t n) {
    uint8_t *a = aa; const uint8_t *b = bb;
    for (size_t i = 0; i < n; ++i) a[i] = b[i];
}

static void *invoke(void *method, void *object, const void **arguments, bool *ok) {
    if (!*ok || !method) { *ok = false; return NULL; }
    const void *exception = NULL;
    void *result = mono_runtime_invoke(method, object, arguments, &exception);
    if (exception) *ok = false;
    return *ok ? result : NULL;
}
static void *setter(void *klass, const char *name) {
    void *property = klass ? mono_class_get_property_from_name(klass, name) : NULL;
    return property ? mono_property_get_set_method(property) : NULL;
}
static void *getter(void *klass, const char *name) {
    void *property = klass ? mono_class_get_property_from_name(klass, name) : NULL;
    return property ? mono_property_get_get_method(property) : NULL;
}
static void set(void *method, void *object, const void *value, bool *ok) {
    const void *arguments[] = {value};
    invoke(method, object, arguments, ok);
}

static bool version_string(const void *value) {
    if (!value || !info.version[0]) return false;
    const HNCYString *text = value;
    size_t n = length(info.version);
    if (text->length != n) return false;
    for (size_t i = 0; i < n; ++i)
        if (text->characters[i] != (uint8_t)info.version[i]) return false;
    return true;
}

static void *find_logo(void *label, bool *ok) {
    void *name = mono_string_new(Root_Domain, logo_name);
    if (!name) { *ok = false; return NULL; }
    const void *arguments[] = {name};
    return invoke(info.find, label, arguments, ok);
}

static bool attach_logo(void *label) {
    bool ok = true;
    if (find_logo(label, &ok)) return ok;
    if (!ok) return false;
    void *image = mono_object_new(Root_Domain, info.image_class);
    if (!image) return false;
    invoke(info.ctor, image, NULL, &ok);
    void *name = mono_string_new(Root_Domain, logo_name);
    void *uri = mono_string_new(Root_Domain, logo_uri);
    if (!name || !uri) return false;
    set(info.name, image, name, &ok);
    float edge = 64.0f, x = 0.0f, y = -8.0f;
    set(info.width, image, &edge, &ok);
    set(info.height, image, &edge, &ok);
    set(info.x, image, &x, &ok);
    set(info.y, image, &y, &ok);
    /* A three-argument overload uses the image loader's default options. */
    const void *load_arguments[] = {uri, NULL, NULL};
    invoke(info.load, image, load_arguments, &ok);
    if (!ok) return false;
    const void *append_arguments[] = {image};
    invoke(info.append, label, append_arguments, &ok);
    if (ok && info.clip) {
        uint8_t clip = 0;
        set(info.clip, label, &clip, &ok);
    }
    if (!ok) {
        bool remove_ok = true;
        invoke(info.remove, image, NULL, &remove_ok);
    }
    return ok;
}

static void text_hook(void *label, void *text) {
    /* All unrelated labels immediately take the existing native setter. */
    bool match = version_string(text);
    if (!match && label == info.last_label && info.image_ready) {
        bool ok = true;
        void *image = find_logo(label, &ok);
        if (image && ok) invoke(info.remove, image, NULL, &ok);
        info.last_label = NULL;
    }
    info.text_original(label, text);
    if (match && label && info.image_ready && attach_logo(label)) {
        info.last_label = label;
        /* Reserve three em spaces for the image, only after attaching it. */
        void *padded = mono_string_new(Root_Domain, info.padded);
        if (padded) info.text_original(label, padded);
    }
}

static size_t relocatable_prologue(const uint8_t *code) {
    size_t size = 0;
    while (size < 14) {
        HNCYInsn instruction = {0};
        unsigned n = hde64_disasm(code + size, &instruction);
        if (!n || n > 15 || size + n > 48 || (instruction.flags & (0x1000u | 0x200u)))
            return 0; /* Decode error or PC-relative branch/call. */
        if ((instruction.flags & 1u) && instruction.modrm_mod == 0 && instruction.modrm_rm == 5)
            return 0; /* RIP-relative memory operand cannot be copied. */
        if (instruction.opcode == 0xc2 || instruction.opcode == 0xc3 ||
            instruction.opcode == 0xca || instruction.opcode == 0xcb ||
            (instruction.opcode == 0xff && instruction.modrm_reg >= 2 && instruction.modrm_reg <= 5))
            return 0;
        size += n;
    }
    return size;
}
static void jump(uint8_t *bytes, uintptr_t destination) {
    const uint8_t prefix[6] = {0xff, 0x25, 0, 0, 0, 0};
    copy_bytes(bytes, prefix, 6);
    copy_bytes(bytes + 6, &destination, 8);
}
static bool install_text_hook(void *method) {
    uint8_t *target = mono_aot_get_method(Root_Domain, method);
    if (!target) return false;
    size_t displaced = relocatable_prologue(target);
    if (!displaced) return false;
    uint8_t trampoline[64] = {0}, patch[14], original[14];
    copy_bytes(trampoline, target, displaced);
    jump(trampoline + displaced, (uintptr_t)target + displaced);
    jump(patch, (uintptr_t)text_hook);
    copy_bytes(original, target, sizeof(original));
    int pid = getpid();
    if (sys_proc_rw(pid, (uintptr_t)hncy_text_trampoline, trampoline, displaced + 14, 1) != 0 ||
        !same_bytes(hncy_text_trampoline, trampoline, displaced + 14)) return false;
    info.text_original = (void (*)(void *, void *))hncy_text_trampoline;
    if (sys_proc_rw(pid, (uintptr_t)target, patch, sizeof(patch), 1) == 0 &&
        same_bytes(target, patch, sizeof(patch))) return true;
    sys_proc_rw(pid, (uintptr_t)target, original, sizeof(original), 1);
    return false;
}

static bool prepare_logo(void) {
    if (info.image_ready) return true;
    const char *sandbox = sceKernelGetFsSandboxRandomWord();
    if (!sandbox || !sandbox[0]) return false;
    char path[260];
    int n = snprintf(path, sizeof(path), "/%s/common/lib/Sce.PlayStation.PUI.dll", sandbox);
    if (n < 0 || (size_t)n >= sizeof(path)) return false;
    void *ui = mono_get_image(path);
    const char *ns = "Sce.PlayStation.PUI.UI2";
    if (!ui) {
        n = snprintf(path, sizeof(path), "/%s/common/lib/Sce.PlayStation.HighLevel.UI2.dll", sandbox);
        if (n < 0 || (size_t)n >= sizeof(path)) return false;
        ui = mono_get_image(path);
        ns = "Sce.PlayStation.HighLevel.UI2";
    }
    if (!ui) return false;
    void *widget = mono_class_from_name(ui, ns, "Widget");
    void *label = mono_class_from_name(ui, ns, "Label");
    info.image_class = mono_class_from_name(ui, ns, "ImageBox");
    if (!widget || !label || !info.image_class) return false;
    info.ctor = mono_class_get_method_from_name(info.image_class, ".ctor", 0);
    info.load = mono_class_get_method_from_name(info.image_class, "LoadAsync", 1);
    info.load_arguments = 1;
    if (!info.load) {
        info.load = mono_class_get_method_from_name(info.image_class, "LoadAsync", 3);
        info.load_arguments = 3;
    }
    info.append = mono_class_get_method_from_name(widget, "AppendChild", 1);
    if (!info.append) info.append = mono_class_get_method_from_name(widget, "AddChildLast", 1);
    info.find = mono_class_get_method_from_name(widget, "FindChildByName", 1);
    if (!info.find) info.find = mono_class_get_method_from_name(widget, "FindWidgetByName", 1);
    info.remove = mono_class_get_method_from_name(widget, "RemoveFromParent", 0);
    info.name = setter(widget, "Name");
    info.width = setter(widget, "Width");
    info.height = setter(widget, "Height");
    info.x = setter(widget, "X"); info.y = setter(widget, "Y");
    info.clip = setter(widget, "ClipChildren");
    void *text = setter(label, "Text");
    if (!info.ctor || !info.load || !info.append || !info.find || !info.remove ||
        !info.name || !info.width || !info.height || !info.x || !info.y || !text) return false;
    return install_text_hook(text);
}

static bool apply_version(void *app) {
    void *klass = mono_class_from_name(app, "Sce.Vsh.ShellUI.AppSystem", "SystemSoftwareVersionInfo");
    if (!klass) return false;
    void *instance_get = getter(klass, "Instance"), *display_get = getter(klass, "DisplayVersion");
    void *display_set = setter(klass, "DisplayVersion");
    if (!instance_get || !display_get || !display_set) return false;
    bool ok = true;
    void *instance = invoke(instance_get, NULL, NULL, &ok);
    if (!instance || !ok) return false;
    void *original = invoke(display_get, instance, NULL, &ok);
    if (!original || !ok) return false;
    char *native = mono_string_to_utf8(original);
    if (!native) return false;
    size_t size = length(native), tail = sizeof(suffix) - 1;
    if (size >= tail && equal(native + size - tail, suffix)) size -= tail;
    int n = -1;
    if (size && size < sizeof(info.version) - sizeof(suffix))
        n = snprintf(info.version, sizeof(info.version), "%.*s%s", (int)size, native, suffix);
    mono_free(native);
    if (n <= 0 || (size_t)n >= sizeof(info.version)) { info.version[0] = 0; return false; }
    n = snprintf(info.padded, sizeof(info.padded), "\xe2\x80\x83\xe2\x80\x83\xe2\x80\x83%s", info.version);
    if (n <= 0 || (size_t)n >= sizeof(info.padded)) return false;
    info.image_ready = prepare_logo();
    void *branded = mono_string_new(Root_Domain, info.version);
    if (!branded) return false;
    set(display_set, instance, branded, &ok);
    if (ok) {
        void *actual = invoke(display_get, instance, NULL, &ok);
        ok = ok && version_string(actual);
    }
    if (!ok) {
        bool restore = true;
        set(display_set, instance, original, &restore);
        info.version[0] = 0;
    }
    return ok;
}

/* Called once on ShellUI's main thread, after its normal boot initialization. */
void HNCY_SystemInfoBoot(void) {
    MainThreadCheck();
    if (!Root_Domain || !App_Exe) return;
    bool applied = apply_version(App_Exe);
    printf("[HNCY] System Information version: %s; logo hook: %s\n",
           applied ? "HENACY v1" : "unavailable", info.image_ready ? "ready" : "unavailable");
}
