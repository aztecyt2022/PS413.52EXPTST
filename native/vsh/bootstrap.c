/* Linked only into unused space in the SHA-pinned HENACY plugin_mono image. */
#include "abi.h"
#define LOCAL __attribute__((visibility("hidden")))
extern LOCAL void HNCY_SystemInfoBoot(void);
extern LOCAL void *Root_Domain, *App_Exe;
extern LOCAL void *mono_get_image(const char *);
extern LOCAL int sys_proc_rw(int, uintptr_t, const void *, size_t, int);
extern LOCAL unsigned hde64_disasm(const void *, VshInstruction *);
extern LOCAL int sceKernelLoadStartModule(const char *, size_t, const void *, unsigned, void *, void *);
extern LOCAL int printf(const char *, ...);

void HNCY_VshBoot(void) {
    HNCY_SystemInfoBoot();
    if (!Root_Domain || !App_Exe) return;
    int module = sceKernelLoadStartModule("/data/hen/henacy_vsh.prx", 0, NULL, 0, NULL, NULL);
    if (module < 0) {
        printf("[HENACY VSH] Install /data/hen/henacy_vsh.prx first (load result %x).\n", module);
        return;
    }
    int (*start)(const HncyVshHost *) = NULL;
    if (sceKernelDlsym(module, "HNCY_VshStart", (void **)&start) == 0 && start) {
        const HncyVshHost api = {sizeof(api), HNCY_VSH_ABI, Root_Domain, App_Exe,
                                mono_get_image, sys_proc_rw, hde64_disasm};
        printf("[HENACY VSH] Native start returned %d.\n", start(&api));
    }
    /* Do not unload: the module owns ShellUI callbacks for this process. */
}
