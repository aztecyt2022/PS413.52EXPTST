#pragma once
#include <stdbool.h>
#include <stdint.h>

/* DualShock 4 button bits from OpenOrbis include/orbis/_types/pad.h. */
enum {
    VSH_L3 = 2, VSH_R3 = 4, VSH_UP = 0x10, VSH_RIGHT = 0x20,
    VSH_DOWN = 0x40, VSH_LEFT = 0x80, VSH_CIRCLE = 0x2000,
    VSH_CROSS = 0x4000, VSH_CHORD = VSH_L3 | VSH_R3
};
enum { VSH_MAIN, VSH_CONSOLE, VSH_OVERLAY, VSH_SERVICES, VSH_SETTINGS, VSH_ABOUT, VSH_PAGES };
enum { VSH_NONE, VSH_SAVE, VSH_RESET, VSH_DISABLE, VSH_SERVICE_TOGGLE };
enum { VSH_FTP, VSH_PAYLOAD, VSH_KLOG, VSH_SERVICE_COUNT };
enum { VSH_SERVICE_UNAVAILABLE, VSH_SERVICE_OFF, VSH_SERVICE_ON, VSH_SERVICE_ERROR };

typedef struct {
    uint8_t stats, side, theme, opacity, fahrenheit;
} VshConfig;

typedef struct {
    VshConfig config;
    uint8_t page, selected[VSH_PAGES], action, requested_service;
    uint8_t service_state[VSH_SERVICE_COUNT];
    bool requested_enable, open, enabled, chord_latched, draining, dirty;
    uint32_t previous, repeat_button;
    uint64_t next_repeat;
    int owner;
} VshMenu;

void vsh_defaults(VshConfig *config);
bool vsh_config_valid(const VshConfig *config);
void vsh_init(VshMenu *menu);
unsigned vsh_rows(unsigned page);
/* Returns whether this sample must be hidden from the underlying XMB. */
bool vsh_input(VshMenu *menu, int handle, uint32_t buttons, bool connected,
               bool home_ready, uint64_t now_us);
void vsh_home_lost(VshMenu *menu);
