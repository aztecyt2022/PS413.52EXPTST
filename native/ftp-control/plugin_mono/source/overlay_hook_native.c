/* Native relay allocation. Never uses MAP_FIXED or overwrites another mapping.
 * No assumed firmware addresses; all branch destinations come from Mono.
 * GPL-3.0-or-later.
 */
#include "overlay_hook.h"
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
extern int sys_proc_rw(int,uintptr_t,const void *,uint64_t,uint64_t);

int dexcore_overlay_hook_install(uintptr_t source,uintptr_t handler,uintptr_t *original)
{
    if(!source || !handler || !original)return 0;
    const size_t page_size=0x4000;
    const int64_t distances[]={0x100000, -0x100000, 0x1000000, -0x1000000,
                               0x4000000, -0x4000000, 0x10000000, -0x10000000,0};
    uint8_t saved[5];memcpy(saved,(const void *)source,sizeof(saved));
    for(size_t i=0;i<sizeof(distances)/sizeof(distances[0]);i++) {
        uintptr_t hint=distances[i]?(uintptr_t)((int64_t)(source&~(uintptr_t)0x3fff)+distances[i]):0;
        uint8_t *page=mmap((void *)hint,page_size,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);
        if(page==MAP_FAILED)continue;
        int64_t delta=(int64_t)(uintptr_t)page-(int64_t)(source+5);
        size_t stolen=0;
        if(delta<INT32_MIN || delta>INT32_MAX ||
           !dexcore_overlay_hook_plan((const uint8_t *)source,source,page+32,(uintptr_t)(page+32),96,&stolen)) {
            munmap(page,page_size);continue;
        }
        const uint8_t jump[]={0xff,0x25,0,0,0,0};
        memcpy(page,jump,sizeof(jump));memcpy(page+6,&handler,sizeof(handler));
        if(mprotect(page,page_size,PROT_READ|PROT_EXEC)) {munmap(page,page_size);continue;}
        uint8_t patch[5]={0xe9};int32_t relative=(int32_t)delta;memcpy(patch+1,&relative,4);
        /* Publish the original entry before any thread can take the new branch. */
        __atomic_store_n(original,(uintptr_t)(page+32),__ATOMIC_RELEASE);
        sys_proc_rw(getpid(),source,patch,sizeof(patch),1);
        if(!memcmp((const void *)source,patch,sizeof(patch)))return 1;
        sys_proc_rw(getpid(),source,saved,sizeof(saved),1);
        if(!memcmp((const void *)source,saved,sizeof(saved)))munmap(page,page_size);
        return 0;
    }
    return 0;
}
