/* Exercise the actual background sampler and persisted toggle, never a PS4. */
#include "system_overlay.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define STATE "/tmp/dexcore-overlay-ci/state"
static int reads,block_cpu,in_flight;
int dexcore_test_cpu(int *out)
{
    __atomic_add_fetch(&reads,1,__ATOMIC_SEQ_CST);
    __atomic_store_n(&in_flight,1,__ATOMIC_SEQ_CST);
    while(__atomic_load_n(&block_cpu,__ATOMIC_SEQ_CST))usleep(1000);
    *out=62;return 0;
}
int dexcore_test_soc(int id,int *out){*out=63+id;return id<2?0:-1;}
int dexcore_test_fan(uint16_t *duty,uint64_t *chassis){*duty=512;*chassis=0;return 0;}
int dexcore_test_sysctl(const char *name,void *out,size_t *length)
{
    uint64_t value=!strcmp(name,"hw.physmem")?8ull*1024*1024*1024:
                   !strcmp(name,"hw.pagesize")?16384:131072;
    assert(*length>=sizeof(value));memcpy(out,&value,sizeof(value));*length=sizeof(value);return 0;
}
static DexcoreOverlayFrame wait_frame(uint32_t after)
{
    DexcoreOverlayFrame value;
    for(int i=0;i<400;i++) {
        if(dexcore_overlay_get_frame(&value) && value.ready && value.revision>after)return value;
        usleep(10000);
    }
    assert(!"timed out waiting for live overlay sample");return value;
}
static void write_state(const char *text)
{
    FILE *file=fopen(STATE,"wb");assert(file);
    assert(fputs(text,file)>=0);assert(!fclose(file));
}
int main(int argc,char **argv)
{
    assert(argc==2);assert(!mkdir("/tmp/dexcore-overlay-ci",0700) || access("/tmp/dexcore-overlay-ci",F_OK)==0);
    unlink(STATE);unlink(STATE ".tmp");rmdir(STATE ".tmp");
    if(!strcmp(argv[1],"persist")) {
        write_state("1\n");assert(dexcore_overlay_start());assert(dexcore_overlay_enabled());
        DexcoreOverlayFrame value=wait_frame(0);assert(value.enabled);
        puts("PASS: overlay On state resumes after native process restart");return 0;
    }
    write_state("invalid\n");assert(dexcore_overlay_start());assert(!dexcore_overlay_enabled());
    usleep(250000);assert(!__atomic_load_n(&reads,__ATOMIC_SEQ_CST));
    assert(dexcore_overlay_set_enabled(1));assert(dexcore_overlay_enabled());
    DexcoreOverlayFrame first=wait_frame(0);
    assert(strstr(first.text,"CPU / APU: 62 C"));assert(strstr(first.text,"SoC Sensor 1: 64 C"));
    assert(strstr(first.text,"Fan Duty: 50%"));assert(strstr(first.text,"RAM Used (Est.): 75%"));
    usleep(350000);assert(__atomic_load_n(&reads,__ATOMIC_SEQ_CST)==1);
    DexcoreOverlayFrame second=wait_frame(first.revision);assert(second.revision>first.revision);
    assert(dexcore_overlay_set_enabled(0));usleep(200000);
    int stopped=__atomic_load_n(&reads,__ATOMIC_SEQ_CST);usleep(400000);
    assert(__atomic_load_n(&reads,__ATOMIC_SEQ_CST)==stopped);
    DexcoreOverlayFrame disabled;assert(dexcore_overlay_get_frame(&disabled));assert(!disabled.enabled && !disabled.ready);
    assert(!mkdir(STATE ".tmp",0700));assert(!dexcore_overlay_set_enabled(1));
    assert(!dexcore_overlay_enabled());assert(!rmdir(STATE ".tmp"));
    /* A sample begun before Off must not resurrect a visible frame. */
    __atomic_store_n(&block_cpu,1,__ATOMIC_SEQ_CST);__atomic_store_n(&in_flight,0,__ATOMIC_SEQ_CST);
    assert(dexcore_overlay_set_enabled(1));
    for(int i=0;i<300 && !__atomic_load_n(&in_flight,__ATOMIC_SEQ_CST);i++)usleep(10000);
    assert(__atomic_load_n(&in_flight,__ATOMIC_SEQ_CST));assert(dexcore_overlay_set_enabled(0));
    __atomic_store_n(&block_cpu,0,__ATOMIC_SEQ_CST);usleep(250000);
    assert(dexcore_overlay_get_frame(&disabled));assert(!disabled.enabled && !disabled.ready);
    FILE *file=fopen(STATE,"rb");assert(file);assert(fgetc(file)=='0');fclose(file);
    puts("PASS: live samples repeat without opening Settings; Off stops reads; failed persistence preserves state; stale samples are discarded");
    return 0;
}
