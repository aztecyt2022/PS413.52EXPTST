#include "menu.h"

void vsh_defaults(VshConfig *c) {
    *c = (VshConfig){.stats = 0, .side = 0, .theme = 3, .opacity = 90, .fahrenheit = 0};
}
bool vsh_config_valid(const VshConfig *c) {
    return c->stats <= 1 && c->side <= 1 && c->theme <= 3 &&
           c->opacity >= 70 && c->opacity <= 100 && c->opacity % 5 == 0 && c->fahrenheit <= 1;
}
void vsh_init(VshMenu *m) {
    *m = (VshMenu){.enabled = true, .dirty = true, .owner = -1};
    vsh_defaults(&m->config);
    /* Never assume a listening port is owned by HENACY: discovery and
       start/stop must be supplied by a real PS4 service backend. */
    for (unsigned i = 0; i < VSH_SERVICE_COUNT; ++i)
        m->service_state[i] = VSH_SERVICE_UNAVAILABLE;
}
unsigned vsh_rows(unsigned page) {
    static const uint8_t rows[] = {7, 5, 3, 4, 6, 4};
    return page < VSH_PAGES ? rows[page] : 0;
}
void vsh_home_lost(VshMenu *m) {
    m->draining |= m->open;
    m->open = false;
    m->previous = m->repeat_button = 0;
    m->dirty = true;
}
static void adjust(VshMenu *m, int direction) {
    unsigned row = m->selected[m->page];
    VshConfig *c = &m->config;
    if (m->page == VSH_OVERLAY) {
        if (row == 0) c->stats ^= 1;
        if (row == 1) c->fahrenheit ^= 1;
        if (row == 2) c->side ^= 1;
    } else if (m->page == VSH_SETTINGS) {
        if (row == 0) c->theme = (uint8_t)((c->theme + 4 + direction) % 4);
        if (row == 1) {
            int value = c->opacity + direction * 5;
            if (value >= 70 && value <= 100) c->opacity = (uint8_t)value;
        }
        if (row == 2) c->side ^= 1;
    }
    m->dirty = true;
}
static void enter(VshMenu *m) {
    unsigned row = m->selected[m->page];
    if (m->page == VSH_MAIN) {
        if (row < 5) m->page = (uint8_t)(row + 1);
        else if (row == 5) { m->enabled = false; m->open = false; m->action = VSH_DISABLE; }
        else m->open = false;
    } else if (m->page == VSH_SERVICES) {
        if (row < VSH_SERVICE_COUNT) {
            m->requested_service = (uint8_t)row;
            m->requested_enable = m->service_state[row] != VSH_SERVICE_ON;
            m->action = VSH_SERVICE_TOGGLE;
        } else m->page = VSH_MAIN;
    } else if (m->page == VSH_SETTINGS && row >= 3) {
        if (row == 3) m->action = VSH_SAVE;
        if (row == 4) { vsh_defaults(&m->config); m->action = VSH_RESET; }
        if (row == 5) m->page = VSH_MAIN;
    } else if (m->page == VSH_OVERLAY || (m->page == VSH_SETTINGS && row < 3)) adjust(m, 1);
    m->dirty = true;
}
bool vsh_input(VshMenu *m, int handle, uint32_t b, bool connected, bool ready, uint64_t now) {
    bool consume = m->open || m->draining;
    if (!connected) {
        if (handle == m->owner) {
            vsh_home_lost(m); m->draining = false; m->owner = -1; m->chord_latched = false;
        }
        return consume;
    }
    if (!m->enabled) {
        if (!b) m->draining = false;
        return consume;
    }
    if (!ready) {
        if (m->open) vsh_home_lost(m);
        if (!b) m->draining = false;
        /* Never open by holding a chord while returning from another screen. */
        m->chord_latched = (b & VSH_CHORD) != 0;
        return consume;
    }
    if (m->open && handle != m->owner) return true;
    if (!(b & VSH_CHORD)) m->chord_latched = false;
    if ((b & VSH_CHORD) == VSH_CHORD && !m->chord_latched) {
        m->owner = handle; m->chord_latched = true;
        m->open = !m->open; m->dirty = true; m->draining = true;
        m->previous = b; m->repeat_button = 0;
        return true;
    }
    if (m->draining) {
        if (!b) { m->draining = false; m->previous = 0; }
        return true;
    }
    if (!m->open) { m->previous = b; return consume; }
    uint32_t pressed = b & ~m->previous;
    uint32_t direction = b & (VSH_UP | VSH_DOWN | VSH_LEFT | VSH_RIGHT);
    if (direction && !(direction & (direction - 1))) {
        if (direction != m->repeat_button) {
            m->repeat_button = direction; m->next_repeat = now + 350000;
        } else if (now >= m->next_repeat) {
            pressed |= direction; m->next_repeat = now + 100000;
        }
    } else m->repeat_button = 0;
    if (pressed & VSH_CIRCLE) {
        if (m->page == VSH_MAIN) m->open = false;
        else m->page = VSH_MAIN;
        m->dirty = true;
    } else if (pressed & VSH_CROSS) enter(m);
    else if (pressed & (VSH_UP | VSH_DOWN)) {
        unsigned count = vsh_rows(m->page), index = m->selected[m->page];
        m->selected[m->page] = (uint8_t)((index + count + ((pressed & VSH_DOWN) ? 1 : -1)) % count);
        m->dirty = true;
    } else if (pressed & (VSH_LEFT | VSH_RIGHT)) adjust(m, (pressed & VSH_RIGHT) ? 1 : -1);
    m->previous = b;
    if (!m->open) m->draining = true;
    return true;
}
