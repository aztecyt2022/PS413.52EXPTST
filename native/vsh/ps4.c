/* HENACY VSH: original PS4 implementation using Akari 1.2.2's menu concept.
 * Mono/PUI integration follows public PS4-Mono and Orbis-Toolbox research.
 * See README.md for provenance and the required PS4 hardware validation.
 */
#include "abi.h"
#include "menu.h"
#include "detour.h"
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

#ifndef CONFIG_PATH
#define CONFIG_PATH "/data/hen/henacy-vsh.cfg"
#define CONFIG_TEMP "/data/hen/henacy-vsh.cfg.tmp"
#define LOG_PATH "/data/hen/henacy-vsh.log"
#endif

static HncyVshHost host;
static VshMenu menu;
static atomic_flag input_lock = ATOMIC_FLAG_INIT;
static atomic_bool home_ready;
static bool started, ui_failed, residency_required;
static VshHook update_hook, read_hook, state_hook;
static VshModuleInfo pad_module;
static uint64_t sample_time;
static unsigned frame_count, update_rate;
static int cpu = -1, soc = -1;
static int (*get_cpu)(int *), (*get_soc)(int, int *);
static uint64_t last_pad_timestamp;
static int last_pad_handle = -1;
static bool last_pad_consumed;
static char status[96] = "Settings are applied immediately. Save to keep them.";

static struct {
    void *panel_class, *label_class, *font_class;
    void *panel_ctor, *label_ctor, *font_ctor;
    void *area_instance, *top_scene, *root_widget, *visible;
    void *width_get, *height_get, *append, *remove;
    void *name, *x, *y, *width, *height, *order, *background;
    void *text, *text_color, *font;
    uint32_t root, panel, highlight, title, subtitle, rows[6], footer, stats;
    unsigned row_count;
    float scale, screen_width, screen_height;
} ui;

static void lock(void) { while (atomic_flag_test_and_set_explicit(&input_lock, memory_order_acquire)) {} }
static void unlock(void) { atomic_flag_clear_explicit(&input_lock, memory_order_release); }
static void log_line(const char *message) {
    printf("[HENACY VSH] %s\n", message);
    FILE *file = fopen(LOG_PATH, "a");
    if (file) { fprintf(file, "%s\n", message); fclose(file); }
}
static void *call(void *method, void *object, const void **args, bool *ok) {
    if (!*ok || !method) { *ok = false; return NULL; }
    const void *exception = NULL;
    void *result = mono_runtime_invoke(method, object, args, &exception);
    if (exception) *ok = false;
    return *ok ? result : NULL;
}
static void *property(void *klass, const char *name, bool write) {
    void *p = klass ? mono_class_get_property_from_name(klass, name) : NULL;
    return !p ? NULL : write ? mono_property_get_set_method(p) : mono_property_get_get_method(p);
}
static void set(void *method, void *object, const void *value, bool *ok) {
    const void *args[] = {value}; call(method, object, args, ok);
}
static void *object(uint32_t handle) { return handle ? mono_gchandle_get_target(handle) : NULL; }
static uint32_t pin(void *value, bool *ok) {
    uint32_t h = value && *ok ? mono_gchandle_new(value, 1) : 0;
    if (!h) *ok = false;
    return h;
}
static void release(uint32_t *handle) {
    if (*handle) { mono_gchandle_free(*handle); *handle = 0; }
}
static void set_text(uint32_t handle, const char *text, bool *ok) {
    void *value = mono_string_new(host.domain, text);
    if (!value) { *ok = false; return; }
    set(ui.text, object(handle), value, ok);
}
static void rect(uint32_t handle, float x, float y, float width, float height, bool *ok) {
    void *w = object(handle);
    x *= ui.scale; y *= ui.scale; width *= ui.scale; height *= ui.scale;
    set(ui.x, w, &x, ok); set(ui.y, w, &y, ok);
    set(ui.width, w, &width, ok); set(ui.height, w, &height, ok);
}
static void color(uint32_t handle, void *setter, float r, float g, float b, float a, bool *ok) {
    const float rgba[] = {r, g, b, a}; set(setter, object(handle), rgba, ok);
}
static void font_size(uint32_t label, int pixels, bool *ok) {
    uint32_t f = pin(mono_object_new(host.domain, ui.font_class), ok);
    int size = (int)(pixels * ui.scale), style = 0, weight = 0;
    if (size < 12) size = 12;
    void *unboxed = mono_object_unbox(object(f));
    if (!unboxed) *ok = false;
    const void *args[] = {&size, &style, &weight};
    call(ui.font_ctor, unboxed, args, ok);
    set(ui.font, object(label), unboxed, ok); release(&f);
}
static uint32_t make_widget(bool label, const char *name, uint32_t parent, bool *ok) {
    void *klass = label ? ui.label_class : ui.panel_class;
    uint32_t h = pin(mono_object_new(host.domain, klass), ok);
    call(label ? ui.label_ctor : ui.panel_ctor, object(h), NULL, ok);
    void *managed_name = mono_string_new(host.domain, name);
    if (!managed_name) *ok = false;
    set(ui.name, object(h), managed_name, ok);
    if (parent) { const void *args[] = {object(h)}; call(ui.append, object(parent), args, ok); }
    if (label && *ok) {
        font_size(h, 24, ok);
        color(h, ui.text_color, 1, 1, 1, 1, ok);
    }
    return h;
}
static void remove_widget(uint32_t *h) {
    if (*h) { bool ok = true; call(ui.remove, object(*h), NULL, &ok); release(h); }
}
static void destroy_panel(void) {
    remove_widget(&ui.panel);
    release(&ui.title); release(&ui.subtitle); release(&ui.footer); release(&ui.highlight);
    for (unsigned i = 0; i < 6; ++i) release(&ui.rows[i]);
    ui.row_count = 0;
}
static void destroy_ui(void) {
    destroy_panel(); remove_widget(&ui.stats); release(&ui.root);
}
static void failure(const char *reason) {
    atomic_store(&home_ready, false);
    lock(); vsh_home_lost(&menu); menu.enabled = false; unlock();
    destroy_ui(); ui_failed = true; log_line(reason);
}

static bool prepare_ui(void) {
    const char *sandbox = sceKernelGetFsSandboxRandomWord();
    if (!sandbox || !*sandbox) return false;
    char path[260];
    int n = snprintf(path, sizeof(path), "/%s/common/lib/Sce.PlayStation.PUI.dll", sandbox);
    if (n <= 0 || (size_t)n >= sizeof(path)) return false;
    void *image = host.get_image(path);
    const char *pui = "Sce.PlayStation.PUI", *ns = "Sce.PlayStation.PUI.UI2";
    if (!image) {
        n = snprintf(path, sizeof(path), "/%s/common/lib/Sce.PlayStation.HighLevel.UI2.dll", sandbox);
        if (n <= 0 || (size_t)n >= sizeof(path)) return false;
        image = host.get_image(path); pui = "Sce.PlayStation.HighLevel.UI2"; ns = pui;
    }
    if (!image) return false;
    void *widget = mono_class_from_name(image, ns, "Widget");
    ui.panel_class = mono_class_from_name(image, ns, "Panel");
    ui.label_class = mono_class_from_name(image, ns, "Label");
    ui.font_class = mono_class_from_name(image, ns, "UIFont");
    void *scene = mono_class_from_name(image, ns, "Scene");
    void *application = mono_class_from_name(image, pui, "Application");
    void *area = mono_class_from_name(host.app, "Sce.Vsh.ShellUI.TopMenu", "AreaManager");
    if (!widget || !ui.panel_class || !ui.label_class || !ui.font_class || !scene || !application || !area) return false;
    ui.panel_ctor = mono_class_get_method_from_name(ui.panel_class, ".ctor", 0);
    ui.label_ctor = mono_class_get_method_from_name(ui.label_class, ".ctor", 0);
    ui.font_ctor = mono_class_get_method_from_name(ui.font_class, ".ctor", 3);
    ui.area_instance = property(area, "Instance", false);
    ui.top_scene = mono_class_get_method_from_name(area, "GetTopScene", 0);
    ui.root_widget = property(scene, "RootWidget", false);
    ui.visible = property(widget, "Visible", false);
    ui.width_get = property(widget, "Width", false); ui.height_get = property(widget, "Height", false);
    ui.name = property(widget, "Name", true); ui.x = property(widget, "X", true); ui.y = property(widget, "Y", true);
    ui.width = property(widget, "Width", true); ui.height = property(widget, "Height", true);
    ui.order = property(ui.panel_class, "RenderingOrder", true);
    ui.background = property(ui.panel_class, "BackgroundColor", true);
    ui.text = property(ui.label_class, "Text", true); ui.font = property(ui.label_class, "Font", true);
    ui.text_color = property(ui.label_class, "TextColor", true);
    ui.append = mono_class_get_method_from_name(widget, "AppendChild", 1);
    ui.remove = mono_class_get_method_from_name(widget, "RemoveFromParent", 0);
    void *required[] = {ui.panel_ctor, ui.label_ctor, ui.font_ctor, ui.area_instance, ui.top_scene,
        ui.root_widget, ui.visible, ui.width_get, ui.height_get, ui.name, ui.x, ui.y, ui.width,
        ui.height, ui.order, ui.background, ui.text, ui.font, ui.text_color, ui.append, ui.remove};
    for (unsigned i = 0; i < sizeof(required) / sizeof(required[0]); ++i) if (!required[i]) return false;
    void *method = mono_class_get_method_from_name(application, "Update", 0);
    return method && vsh_hook_prepare(&update_hook, mono_aot_get_method(host.domain, method), &host);
}

static void read_config(void) {
    FILE *file = fopen(CONFIG_PATH, "r");
    if (!file) return;
    unsigned stats, side, theme, opacity, units;
    int fields = fscanf(file, "HENACY_VSH_1\nstats=%u\nside=%u\ntheme=%u\nopacity=%u\nfahrenheit=%u\n",
                        &stats, &side, &theme, &opacity, &units);
    int extra = fgetc(file); fclose(file);
    if (fields != 5 || extra != EOF || stats > 1 || side > 1 || theme > 2 || opacity > 100 || units > 1) {
        log_line("Configuration rejected; using defaults."); return;
    }
    VshConfig c = {(uint8_t)stats, (uint8_t)side, (uint8_t)theme, (uint8_t)opacity, (uint8_t)units};
    if (vsh_config_valid(&c)) menu.config = c;
}
static bool save_config(const VshConfig *c) {
    if (!vsh_config_valid(c)) return false;
    FILE *f = fopen(CONFIG_TEMP, "w");
    if (!f) return false;
    bool ok = fprintf(f, "HENACY_VSH_1\nstats=%u\nside=%u\ntheme=%u\nopacity=%u\nfahrenheit=%u\n",
                      c->stats, c->side, c->theme, c->opacity, c->fahrenheit) > 0;
    if (fclose(f) != 0) ok = false;
    if (ok && rename(CONFIG_TEMP, CONFIG_PATH) == 0) return true;
    remove(CONFIG_TEMP); return false;
}

static bool module_contains(const void *pointer, size_t size, bool execute) {
    uintptr_t value = (uintptr_t)pointer;
    if (value + size < value) return false;
    for (unsigned i = 0; i < pad_module.segment_count && i < 4; ++i) {
        const VshSegment *s = &pad_module.segments[i];
        uintptr_t start = (uintptr_t)s->address;
        if (value >= start && value + size <= start + s->size &&
            (s->protection & 1) && (!execute || (s->protection & 4))) return true;
    }
    return false;
}
static void *pad_function(void *address) {
    uint8_t *p = address;
    for (unsigned i = 0; i < 4; ++i) {
        if (!module_contains(p, 64, true)) return NULL;
        if (p[0] == 0xe9) { int32_t offset; memcpy(&offset, p + 1, 4); p += 5 + offset; }
        else if (p[0] == 0xff && p[1] == 0x25) {
            int32_t offset; memcpy(&offset, p + 2, 4); void *slot = p + 6 + offset;
            if (!module_contains(slot, sizeof(p), false)) return NULL;
            memcpy(&p, slot, sizeof(p));
        } else return p;
    }
    return NULL;
}
static bool prepare_pad(void) {
    int modules[256]; size_t count = 0;
    if (sceKernelGetModuleList(modules, sizeof(modules), &count) != 0 || count > 256) return false;
    for (size_t i = 0; i < count; ++i) {
        VshModuleInfo info = {.size = sizeof(info)};
        if (sceKernelGetModuleInfo(modules[i], &info) != 0 ||
            (!strstr(info.name, "libScePad") && strcmp(info.name, "ScePad"))) continue;
        pad_module = info;
        void *read = NULL, *state = NULL;
        if (sceKernelDlsym(modules[i], "scePadRead", &read) ||
            sceKernelDlsym(modules[i], "scePadReadState", &state)) return false;
        read = pad_function(read); state = pad_function(state);
        if (!read || !state || read == state) return false;
        return vsh_hook_prepare(&read_hook, read, &host) && vsh_hook_prepare(&state_hook, state, &host);
    }
    return false;
}
static void filter_pad(int handle, VshPadData *p) {
    lock();
    /* ReadState may internally call Read. Never process the same sample twice. */
    bool consume;
    if (p->timestamp && p->timestamp == last_pad_timestamp && handle == last_pad_handle)
        consume = last_pad_consumed;
    else {
        consume = vsh_input(&menu, handle, p->buttons, p->connected != 0,
                            atomic_load(&home_ready), sceKernelGetProcessTime());
        last_pad_timestamp = p->timestamp; last_pad_handle = handle; last_pad_consumed = consume;
    }
    unlock();
    if (consume) {
        p->buttons = 0;
        p->left[0] = p->left[1] = p->right[0] = p->right[1] = 128;
        p->triggers[0] = p->triggers[1] = 0;
        memset(p->touch, 0, sizeof(p->touch));
    }
}
static int pad_read(int handle, VshPadData *data, int count) {
    int result = ((int (*)(int, VshPadData *, int))read_hook.trampoline)(handle, data, count);
    if (data && result > 0 && result <= count && result <= 64)
        for (int i = 0; i < result; ++i) filter_pad(handle, &data[i]);
    if (result < 0) {
        lock(); vsh_input(&menu, handle, 0, false, false, 0); unlock();
    }
    return result;
}
static int pad_state(int handle, VshPadData *data) {
    int result = ((int (*)(int, VshPadData *))state_hook.trampoline)(handle, data);
    if (result == 0 && data) filter_pad(handle, data);
    if (result < 0) {
        lock(); vsh_input(&menu, handle, 0, false, false, 0); unlock();
    }
    return result;
}

static bool current_root(void) {
    bool ok = true;
    void *area = call(ui.area_instance, NULL, NULL, &ok);
    if (!ok || !area) return false;
    void *scene = call(ui.top_scene, area, NULL, &ok);
    if (!ok || !scene) return false;
    void *root = call(ui.root_widget, scene, NULL, &ok);
    if (!ok || !root) return false;
    void *value = call(ui.visible, root, NULL, &ok);
    if (!ok || !value || !*(uint8_t *)mono_object_unbox(value)) return false;
    if (root != object(ui.root)) {
        atomic_store(&home_ready, false);
        lock(); vsh_home_lost(&menu); unlock();
        destroy_ui(); ui.root = pin(root, &ok);
        value = call(ui.width_get, root, NULL, &ok);
        if (!ok || !value) return false;
        ui.screen_width = *(float *)mono_object_unbox(value);
        value = call(ui.height_get, root, NULL, &ok);
        if (!ok || !value) return false;
        ui.screen_height = *(float *)mono_object_unbox(value);
        if (!(ui.screen_width >= 640 && ui.screen_width <= 7680 && ui.screen_height >= 360 && ui.screen_height <= 4320)) return false;
        ui.scale = ui.screen_width / 1920;
        if (ui.screen_height / 1080 < ui.scale) ui.scale = ui.screen_height / 1080;
    }
    return ok;
}
static void sample_stats(uint64_t now) {
    ++frame_count;
    if (now - sample_time < 1000000) return;
    update_rate = (unsigned)((uint64_t)frame_count * 1000000 / (now - sample_time));
    frame_count = 0; sample_time = now;
    int c = -1, s = -1;
    if (get_cpu && get_cpu(&c) == 0 && c >= 0 && c <= 125) cpu = c; else cpu = -1;
    if (get_soc && get_soc(0, &s) == 0 && s >= 0 && s <= 125) soc = s; else soc = -1;
}
static void temperature(char *out, size_t size, int value, bool fahrenheit) {
    if (value < 0) snprintf(out, size, "unavailable");
    else snprintf(out, size, "%d %s", fahrenheit ? value * 9 / 5 + 32 : value, fahrenheit ? "F" : "C");
}
static void row_text(const VshMenu *m, unsigned row, char *text, size_t capacity) {
    static const char *main[] = {"Console manager  >", "Overlay  >", "Settings  >", "About  >", "Disable until reboot", "Close menu"};
    static const char *themes[] = {"Crimson", "Ocean", "Monochrome"};
    char c[32], s[32]; temperature(c, sizeof(c), cpu, m->config.fahrenheit); temperature(s, sizeof(s), soc, m->config.fahrenheit);
    text[0] = 0;
    if (m->page == VSH_MAIN) snprintf(text, capacity, "%s", main[row]);
    if (m->page == VSH_CONSOLE) {
        if (row == 0) snprintf(text, capacity, "PS4 firmware: 13.52");
        if (row == 1) snprintf(text, capacity, "CPU temperature: %s", c);
        if (row == 2) snprintf(text, capacity, "SoC sensor: %s", s);
        if (row == 3) snprintf(text, capacity, "ShellUI: %u updates/s", update_rate);
        if (row == 4) snprintf(text, capacity, "UI canvas: %.0f x %.0f", (double)ui.screen_width, (double)ui.screen_height);
    }
    if (m->page == VSH_OVERLAY) {
        if (row == 0) snprintf(text, capacity, "Show statistics: %s", m->config.stats ? "On" : "Off");
        if (row == 1) snprintf(text, capacity, "Temperature unit: %s", m->config.fahrenheit ? "Fahrenheit" : "Celsius");
        if (row == 2) snprintf(text, capacity, "Position: %s", m->config.side ? "Right" : "Left");
    }
    if (m->page == VSH_SETTINGS) {
        if (row == 0) snprintf(text, capacity, "Theme: %s", themes[m->config.theme]);
        if (row == 1) snprintf(text, capacity, "Panel opacity: %u%%", m->config.opacity);
        if (row == 2) snprintf(text, capacity, "Position: %s", m->config.side ? "Right" : "Left");
        if (row == 3) snprintf(text, capacity, "Save settings");
        if (row == 4) snprintf(text, capacity, "Reset settings");
        if (row == 5) snprintf(text, capacity, "Back to main menu");
    }
    if (m->page == VSH_ABOUT) {
        if (row == 0) snprintf(text, capacity, "HENACY VSH 0.1 - PS4 test build");
        if (row == 1) snprintf(text, capacity, "Baseline: Akari v1.2.2");
        if (row == 2) snprintf(text, capacity, "Akari author: Jordy Nateur");
        if (row == 3) snprintf(text, capacity, "PS4 screenshots: use SHARE");
    }
}
static void render_panel(const VshMenu *m, bool *ok) {
    static const char *titles[] = {"Main menu", "Console manager", "Overlay", "Settings", "About"};
    static const float themes[][3] = {{0.78f, 0.06f, 0.14f}, {0.03f, 0.32f, 0.72f}, {0.35f, 0.35f, 0.38f}};
    if (!m->open) { destroy_panel(); return; }
    unsigned rows = vsh_rows(m->page);
    if (rows != ui.row_count) destroy_panel();
    if (!ui.panel) {
        ui.panel = make_widget(false, "henacy_vsh_panel", ui.root, ok);
        int order = 1; set(ui.order, object(ui.panel), &order, ok);
        ui.highlight = make_widget(false, "henacy_vsh_selection", ui.panel, ok);
        ui.title = make_widget(true, "henacy_vsh_title", ui.panel, ok);
        ui.subtitle = make_widget(true, "henacy_vsh_subtitle", ui.panel, ok);
        for (unsigned i = 0; i < rows; ++i) ui.rows[i] = make_widget(true, "henacy_vsh_row", ui.panel, ok);
        ui.footer = make_widget(true, "henacy_vsh_help", ui.panel, ok);
        font_size(ui.title, 32, ok); font_size(ui.footer, 16, ok);
        ui.row_count = rows;
    }
    float x = m->config.side ? ui.screen_width / ui.scale - 568 : 48;
    rect(ui.panel, x, 120, 520, 570, ok);
    color(ui.panel, ui.background, 0.025f, 0.025f, 0.04f, m->config.opacity / 100.0f, ok);
    const float *t = themes[m->config.theme];
    rect(ui.highlight, 12, 110 + m->selected[m->page] * 54, 496, 50, ok);
    color(ui.highlight, ui.background, t[0], t[1], t[2], 0.95f, ok);
    rect(ui.title, 24, 20, 480, 36, ok); set_text(ui.title, "HENACY v1 | VSH", ok);
    color(ui.title, ui.text_color, 1, 0.45f, 0.48f, 1, ok);
    rect(ui.subtitle, 24, 62, 480, 36, ok); set_text(ui.subtitle, titles[m->page], ok);
    for (unsigned i = 0; i < rows; ++i) {
        char text[128]; row_text(m, i, text, sizeof(text));
        rect(ui.rows[i], 24, 120 + i * 54, 480, 38, ok); set_text(ui.rows[i], text, ok);
    }
    char footer[256];
    snprintf(footer, sizeof(footer), "D-pad: move/change | Cross: select\nCircle: back | L3 + R3: close\n%s", status);
    rect(ui.footer, 20, 452, 488, 110, ok); set_text(ui.footer, footer, ok);
}
static void render_stats(const VshMenu *m, bool *ok) {
    if (!m->enabled || !m->config.stats) { remove_widget(&ui.stats); return; }
    if (!ui.stats) {
        ui.stats = make_widget(true, "henacy_vsh_stats", ui.root, ok);
        font_size(ui.stats, 18, ok);
    }
    char c[32], s[32], text[160];
    temperature(c, sizeof(c), cpu, m->config.fahrenheit); temperature(s, sizeof(s), soc, m->config.fahrenheit);
    snprintf(text, sizeof(text), "HENACY v1 | CPU %s | SoC %s\nShellUI %u updates/s | L3 + R3: menu", c, s, update_rate);
    float x = m->config.side ? ui.screen_width / ui.scale - 760 : 48;
    rect(ui.stats, x, 24, 720, 82, ok); set_text(ui.stats, text, ok);
}
static void update(void *application) {
    ((void (*)(void *))update_hook.trampoline)(application);
    if (ui_failed) return;
    uint64_t now = sceKernelGetProcessTime(), old_sample = sample_time;
    sample_stats(now);
    if (!current_root()) {
        atomic_store(&home_ready, false);
        lock(); vsh_home_lost(&menu); unlock();
        destroy_ui(); return;
    }
    lock();
    VshMenu snapshot = menu; menu.dirty = false; menu.action = VSH_NONE;
    unlock();
    bool ok = true;
    /* Verify widget construction before enabling interception of XMB input. */
    if (!atomic_load(&home_ready) && snapshot.enabled) {
        uint32_t probe = make_widget(true, "henacy_vsh_probe", ui.root, &ok);
        remove_widget(&probe);
        if (!ok) { failure("PUI widget creation failed; input remains untouched."); return; }
        atomic_store(&home_ready, true);
    }
    if (snapshot.action == VSH_SAVE || snapshot.action == VSH_RESET) {
        snprintf(status, sizeof(status), "%s", save_config(&snapshot.config) ? "Settings saved." : "Could not save settings. Changes last until reboot.");
    }
    if (!snapshot.enabled) {
        atomic_store(&home_ready, false); destroy_ui(); ui_failed = true;
        log_line("Disabled until ShellUI restarts. Resident hooks now pass input through."); return;
    }
    if (snapshot.dirty || old_sample != sample_time) {
        render_panel(&snapshot, &ok); render_stats(&snapshot, &ok);
        if (!ok) failure("PUI rendering failed; menu disabled and controller input released.");
    }
}

/* Kept resident for ShellUI's lifetime: callbacks must never outlive this PRX. */
VSH_EXPORT int HNCY_VshStart(const HncyVshHost *api) {
    if (started) return 0;
    if (!api || api->size != sizeof(*api) || api->abi != HNCY_VSH_ABI ||
        !api->domain || !api->app || !api->get_image || !api->proc_rw || !api->decode) return -1;
    VshFirmware firmware = {.size = sizeof(firmware)};
    if (sceKernelGetSystemSwVersion(&firmware) || (firmware.version >> 16) != 0x1352) {
        log_line("Unsupported firmware: this build requires PS4 13.52."); return -2;
    }
    host = *api; vsh_init(&menu); read_config();
    sceKernelDlsym(0x2001, "sceKernelGetCpuTemperature", (void **)&get_cpu);
    sceKernelDlsym(0x2001, "sceKernelGetSocSensorTemperature", (void **)&get_soc);
    if (!prepare_ui() || !prepare_pad()) {
        log_line("Required PUI API or relocatable controller method unavailable; no hooks installed."); return -3;
    }
    residency_required = true;
    if (!vsh_hook_install(&update_hook, update, &host) ||
        !vsh_hook_install(&read_hook, pad_read, &host) ||
        !vsh_hook_install(&state_hook, pad_state, &host)) {
        vsh_hook_restore(&state_hook, &host); vsh_hook_restore(&read_hook, &host); vsh_hook_restore(&update_hook, &host);
        log_line("Hook installation failed; restoration attempted, module remains resident."); return -4;
    }
    started = true;
    log_line("Native PS4 VSH test build started. On the home screen press L3 + R3.");
    return 0;
}
VSH_EXPORT int _init(void) { return 0; }
VSH_EXPORT int _fini(void) { return residency_required ? -1 : 0; }
