#include <assert.h>
#include <stdio.h>
#include "../native/vsh/menu.h"

static uint64_t now;
static bool input(VshMenu *m, uint32_t b) { now += 16000; return vsh_input(m, 10, b, true, true, now); }
static void tap(VshMenu *m, uint32_t b) { input(m, b); input(m, 0); }
static void open_menu(VshMenu *m) {
    assert(input(m, VSH_CHORD)); assert(m->open);
    input(m, 0);
}
int main(void) {
    VshMenu m;
    vsh_init(&m);
    assert(!input(&m, VSH_CROSS)); input(&m, 0);
    assert(!vsh_input(&m, 10, VSH_CHORD, true, false, ++now)); assert(!m.open);
    /* Holding an open chord while returning home cannot open the menu. */
    input(&m, VSH_CHORD); assert(!m.open); input(&m, 0);
    assert(input(&m, VSH_CHORD)); assert(m.open);
    for (unsigned i = 0; i < 200; ++i) input(&m, VSH_CHORD);
    assert(m.open); input(&m, 0);
    tap(&m, VSH_UP); assert(m.selected[VSH_MAIN] == 5);
    tap(&m, VSH_DOWN); assert(m.selected[VSH_MAIN] == 0);
    tap(&m, VSH_CROSS); assert(m.page == VSH_CONSOLE);
    tap(&m, VSH_CIRCLE); assert(m.page == VSH_MAIN && m.open);
    tap(&m, VSH_DOWN); tap(&m, VSH_CROSS); assert(m.page == VSH_OVERLAY);
    assert(!m.config.stats); tap(&m, VSH_CROSS); assert(m.config.stats);
    /* Holding Cross cannot toggle a setting repeatedly. */
    input(&m, VSH_CROSS); assert(!m.config.stats);
    for (unsigned i = 0; i < 100; ++i) input(&m, VSH_CROSS);
    assert(!m.config.stats); input(&m, 0);
    tap(&m, VSH_CIRCLE); tap(&m, VSH_DOWN); tap(&m, VSH_CROSS);
    assert(m.page == VSH_SERVICES);
    assert(m.service_state[VSH_FTP] == VSH_SERVICE_UNAVAILABLE);
    tap(&m, VSH_CROSS);
    assert(m.action == VSH_SERVICE_TOGGLE && m.requested_service == VSH_FTP && m.requested_enable);
    assert(m.service_state[VSH_FTP] == VSH_SERVICE_UNAVAILABLE); /* No fake activation. */
    m.action = VSH_NONE;
    tap(&m, VSH_CIRCLE); tap(&m, VSH_DOWN); tap(&m, VSH_CROSS);
    assert(m.page == VSH_SETTINGS);
    tap(&m, VSH_LEFT); assert(m.config.theme == 2);
    tap(&m, VSH_RIGHT); assert(m.config.theme == 3);
    tap(&m, VSH_CROSS); assert(m.config.theme == 0);
    tap(&m, VSH_DOWN);
    for (unsigned i = 0; i < 50; ++i) tap(&m, VSH_LEFT);
    assert(m.config.opacity == 70);
    for (unsigned i = 0; i < 50; ++i) tap(&m, VSH_RIGHT);
    assert(m.config.opacity == 100 && vsh_config_valid(&m.config));
    tap(&m, VSH_DOWN); tap(&m, VSH_DOWN); tap(&m, VSH_CROSS);
    assert(m.action == VSH_SAVE); m.action = VSH_NONE;
    tap(&m, VSH_DOWN); tap(&m, VSH_CROSS);
    assert(m.action == VSH_RESET && m.config.opacity == 90 && m.config.theme == 3);
    assert(vsh_input(&m, 11, VSH_DOWN, true, true, now));
    assert(m.selected[VSH_SETTINGS] == 4); /* Other controllers cannot navigate. */
    vsh_input(&m, 10, 0, false, true, now);
    assert(!m.open && m.owner == -1);
    open_menu(&m); vsh_home_lost(&m); assert(!m.open);
    input(&m, 0); open_menu(&m);
    tap(&m, VSH_CIRCLE); assert(m.page == VSH_MAIN);
    input(&m, VSH_CIRCLE); assert(!m.open);
    assert(input(&m, VSH_CIRCLE)); input(&m, 0); /* Closing press cannot leak to XMB. */
    assert(!input(&m, VSH_CROSS)); input(&m, 0);
    vsh_init(&m); open_menu(&m);
    for (unsigned i = 0; i < 5; ++i) tap(&m, VSH_DOWN);
    input(&m, VSH_CROSS); assert(!m.enabled && !m.open);
    assert(input(&m, VSH_CROSS)); input(&m, 0); assert(!input(&m, VSH_CHORD));
    /* Repeat starts after 350 ms and runs at 100 ms, independent of frame rate. */
    vsh_init(&m); open_menu(&m);
    vsh_input(&m, 10, VSH_DOWN, true, true, 1000000); assert(m.selected[0] == 1);
    vsh_input(&m, 10, VSH_DOWN, true, true, 1349999); assert(m.selected[0] == 1);
    vsh_input(&m, 10, VSH_DOWN, true, true, 1350000); assert(m.selected[0] == 2);
    vsh_input(&m, 10, VSH_DOWN, true, true, 1450000); assert(m.selected[0] == 3);
    puts("PASS: VSH 1.2 navigation, fail-closed server requests, VHS theme, controller ownership, repeat timing and disable.");
    return 0;
}
