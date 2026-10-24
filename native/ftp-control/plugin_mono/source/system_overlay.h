/* DEXCORE native System Information overlay, GPL-3.0-or-later. */
#ifndef DEXCORE_SYSTEM_OVERLAY_H
#define DEXCORE_SYSTEM_OVERLAY_H
#include <stdint.h>
#include "system_metrics.h"
#ifdef __cplusplus
extern "C" {
#endif
#define DEXCORE_OVERLAY_TEXT_SIZE 1024
typedef struct DexcoreOverlayFrame {
    uint32_t revision;
    int enabled;
    int ready;
    DexcoreMetrics metrics;
    char text[DEXCORE_OVERLAY_TEXT_SIZE];
} DexcoreOverlayFrame;
int dexcore_overlay_start(void);
int dexcore_overlay_enabled(void);
int dexcore_overlay_set_enabled(int enabled);
int dexcore_overlay_get_frame(DexcoreOverlayFrame *out);
void dexcore_overlay_ui_install(void);
int dexcore_overlay_ui_installed(void);
void dexcore_overlay_ui_queue_toggle(const void *element);
const char *dexcore_overlay_ui_status(void);
#ifdef __cplusplus
}
#endif
#endif
