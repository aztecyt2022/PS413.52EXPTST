/* DEXCORE x86-64 UI hook relocation, GPL-3.0-or-later. */
#ifndef DEXCORE_OVERLAY_HOOK_H
#define DEXCORE_OVERLAY_HOOK_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int dexcore_overlay_hook_plan(const uint8_t *source,uintptr_t source_address,
    uint8_t *trampoline,uintptr_t trampoline_address,size_t capacity,size_t *stolen);
int dexcore_overlay_hook_install(uintptr_t source,uintptr_t handler,uintptr_t *original);
#ifdef __cplusplus
}
#endif
#endif
