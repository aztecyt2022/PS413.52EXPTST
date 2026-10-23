#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HNCY_VSH_ABI 1u
#define VSH_EXPORT __attribute__((visibility("default")))
typedef struct __attribute__((packed)) {
    uint8_t len, p_rep, p_lock, p_seg, p_66, p_67, rex, rex_w, rex_r, rex_x,
            rex_b, opcode, opcode2, modrm, modrm_mod, modrm_reg, modrm_rm,
            sib, sib_scale, sib_index, sib_base;
    uint64_t immediate;
    uint32_t displacement, flags;
} VshInstruction;
_Static_assert(sizeof(VshInstruction) == 37, "HDE64 ABI");

typedef struct {
    uint32_t size, abi;
    void *domain, *app;
    void *(*get_image)(const char *);
    int (*proc_rw)(int, uintptr_t, const void *, size_t, int);
    unsigned (*decode)(const void *, VshInstruction *);
} HncyVshHost;

/* PS4 SDK/OpenOrbis ABI. No PS3 imports, syscalls, addresses or pad layouts. */
typedef struct {
    uint32_t buttons;
    uint8_t left[2], right[2], triggers[2];
    uint16_t padding;
    float orientation[4], acceleration[3], velocity[3];
    uint8_t touch[24], connected;
    uint64_t timestamp;
    uint8_t extension[16], count, unknown[15];
} VshPadData;
_Static_assert(sizeof(VshPadData) == 120, "PS4 ScePadData size");
_Static_assert(offsetof(VshPadData, connected) == 76, "PS4 connected offset");
_Static_assert(offsetof(VshPadData, timestamp) == 80, "PS4 timestamp offset");
typedef struct { void *address; uint32_t size; int32_t protection; } VshSegment;
typedef struct {
    size_t size;
    char name[256];
    VshSegment segments[4];
    uint32_t segment_count;
    uint8_t fingerprint[20];
} VshModuleInfo;
typedef struct { size_t size; char text[28]; uint32_t version; } VshFirmware;

extern int sceKernelDlsym(int, const char *, void **);
extern int sceKernelGetModuleList(int *, size_t, size_t *);
extern int sceKernelGetModuleInfo(int, VshModuleInfo *);
extern int sceKernelGetSystemSwVersion(VshFirmware *);
extern const char *sceKernelGetFsSandboxRandomWord(void);
extern uint64_t sceKernelGetProcessTime(void);

extern void *mono_class_from_name(const void *, const char *, const char *);
extern void *mono_class_get_method_from_name(const void *, const char *, int);
extern void *mono_class_get_property_from_name(const void *, const char *);
extern void *mono_property_get_get_method(const void *);
extern void *mono_property_get_set_method(const void *);
extern void *mono_aot_get_method(const void *, const void *);
extern void *mono_runtime_invoke(const void *, const void *, const void **, const void **);
extern void *mono_object_new(const void *, const void *);
extern void *mono_object_unbox(void *);
extern void *mono_string_new(const void *, const char *);
extern uint32_t mono_gchandle_new(void *, int);
extern void *mono_gchandle_get_target(uint32_t);
extern void mono_gchandle_free(uint32_t);

VSH_EXPORT int HNCY_VshStart(const HncyVshHost *host);
