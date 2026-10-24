/* Execute relocated x86-64 functions, including RIP and relative calls. */
#define _GNU_SOURCE
#include "overlay_hook.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
static uintptr_t original;
static int reject_write;
int sys_proc_rw(int pid,uintptr_t address,const void *data,uint64_t bytes,uint64_t write)
{
    (void)pid;assert(write==1);
    if(reject_write)return -1;
    memcpy((void*)address,data,(size_t)bytes);
    return (int)bytes; /* Successful native transports may report byte counts. */
}
static int replacement(void){return ((int(*)(void))original)()+1;}
static void check(uint8_t *code,uint8_t *trampoline,int expected,size_t wanted)
{
    size_t stolen=0;
    assert(dexcore_overlay_hook_plan(code,(uintptr_t)code,trampoline,(uintptr_t)trampoline,96,&stolen));
    assert(stolen==wanted);
    assert(((int(*)(void))code)()==expected);
    assert(((int(*)(void))trampoline)()==expected);
}
int main(void)
{
    uint8_t *page=mmap(NULL,4096,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANON,-1,0);
    assert(page!=MAP_FAILED);memset(page,0x90,4096);
    uint8_t *out=page+512;int32_t value=42,disp;
    /* RIP-relative load in the first instruction: the old 14-byte guard rejected it. */
    uint8_t load[]={0x8b,0x05,0,0,0,0,0xc3};memcpy(page,load,sizeof(load));
    disp=128-6;memcpy(page+2,&disp,4);memcpy(page+128,&value,4);check(page,out,42,6);
    /* Ordinary six-byte stack prologue followed by RIP data outside the stolen block. */
    uint8_t prologue[]={0x55,0x48,0x89,0xe5,0x41,0x57,0x8b,0x05,0,0,0,0,0x41,0x5f,0x5d,0xc3};
    memcpy(page,prologue,sizeof(prologue));disp=128-12;memcpy(page+8,&disp,4);check(page,out,42,6);
    /* A call/jump stub must retain its original destination after relocation. */
    uint8_t called[]={0xb8,42,0,0,0,0xc3};memcpy(page+256,called,sizeof(called));
    page[0]=0xe8;disp=256-5;memcpy(page+1,&disp,4);page[5]=0xc3;check(page,out,42,5);
    page[0]=0xe9;check(page,out,42,5);
    /* RIP memory operand with a trailing immediate. */
    uint8_t add[]={0x83,0x05,0,0,0,0,1,0x8b,0x05,0,0,0,0,0xc3};
    memcpy(page,add,sizeof(add));disp=128-7;memcpy(page+2,&disp,4);disp=128-13;memcpy(page+9,&disp,4);
    size_t stolen=0;assert(dexcore_overlay_hook_plan(page,(uintptr_t)page,out,(uintptr_t)out,96,&stolen));
    memcpy(page+128,&value,4);assert(((int(*)(void))out)()==43);
    assert(!dexcore_overlay_hook_plan(page,(uintptr_t)page,out,(uintptr_t)page+0x90000000ull,96,&stolen));
    memcpy(page,called,sizeof(called));
    reject_write=1;assert(!dexcore_overlay_hook_install((uintptr_t)page,(uintptr_t)replacement,&original));
    assert(((int(*)(void))page)()==42);reject_write=0;
    assert(dexcore_overlay_hook_install((uintptr_t)page,(uintptr_t)replacement,&original));
    assert(((int(*)(void))page)()==43);
    munmap(page,4096);
    puts("PASS: actual relay installer executes the handler/original path; relocations preserve RIP data, prologues and call/jump stubs; failed writes leave the original entry intact");
    return 0;
}
