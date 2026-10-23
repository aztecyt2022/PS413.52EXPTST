#pragma once
#include "abi.h"

typedef struct {
    uint8_t *target, *trampoline;
    uint8_t original[48];
    size_t length;
    bool installed;
} VshHook;

bool vsh_hook_prepare(VshHook *hook, void *target, const HncyVshHost *host);
bool vsh_hook_install(VshHook *hook, void *replacement, const HncyVshHost *host);
bool vsh_hook_restore(VshHook *hook, const HncyVshHost *host);
