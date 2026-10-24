/* DEXCORE live native overlay sampler. GPL-3.0-or-later.
 * No Mono/UI calls on this thread; rendering only consumes cached snapshots.
 * Off performs no sensor reads. State persists using the existing toggle file.
 */
#include "system_overlay.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#ifndef DEXCORE_OVERLAY_STATE
#define DEXCORE_OVERLAY_STATE "/data/hen/system_information_enabled"
#endif
#define DEXCORE_OVERLAY_TEMP DEXCORE_OVERLAY_STATE ".tmp"

static pthread_mutex_t state_lock=PTHREAD_MUTEX_INITIALIZER;
static int started=0;
static int requested=0;
static uint32_t epoch=0;
static DexcoreOverlayFrame frame;

int dexcore_overlay_enabled(void)
{
    return __atomic_load_n(&requested,__ATOMIC_ACQUIRE);
}

static void format_frame(DexcoreOverlayFrame *value)
{
    static const char *ids[]={"system_info_cpu","system_info_soc0","system_info_soc1",
                              "system_info_soc","system_info_fan","system_info_ram"};
    size_t used=(size_t)snprintf(value->text,sizeof(value->text),"DEXCORE | System Information\n");
    for(unsigned i=0;i<sizeof(ids)/sizeof(ids[0]);i++) {
        char row[144];
        dexcore_metrics_format(&value->metrics,ids[i],row,sizeof(row));
        if(used>=sizeof(value->text))break;
        int n=snprintf(value->text+used,sizeof(value->text)-used,"%s%s",row,i==5?"":"\n");
        if(n<0)break;
        used+=(size_t)n;
    }
}

static void *sampler(void *unused)
{
    (void)unused;
    unsigned wait=0;
    for(;;) {
        if(dexcore_overlay_enabled()) {
            if(!wait) {
                uint32_t before=__atomic_load_n(&epoch,__ATOMIC_ACQUIRE);
                DexcoreOverlayFrame next;
                memset(&next,0,sizeof(next));
                dexcore_metrics_read(&next.metrics);
                next.enabled=1;next.ready=1;
                format_frame(&next);
                pthread_mutex_lock(&state_lock);
                /* Never publish an in-flight sample across an Off/On change. */
                if(dexcore_overlay_enabled() && before==__atomic_load_n(&epoch,__ATOMIC_ACQUIRE)) {
                    next.revision=frame.revision+1;
                    frame=next;
                }
                pthread_mutex_unlock(&state_lock);
                wait=10;
            }
            --wait;
        } else wait=0;
        usleep(100000);
    }
    return NULL;
}

int dexcore_overlay_start(void)
{
    pthread_mutex_lock(&state_lock);
    if(started){pthread_mutex_unlock(&state_lock);return 1;}
    FILE *file=fopen(DEXCORE_OVERLAY_STATE,"rb");
    int enabled=file && fgetc(file)=='1';
    if(file)fclose(file);
    memset(&frame,0,sizeof(frame));
    frame.enabled=enabled;
    snprintf(frame.text,sizeof(frame.text),"DEXCORE | System Information\nCollecting sensor readings...");
    __atomic_store_n(&requested,enabled,__ATOMIC_RELEASE);
    pthread_t thread;
    if(pthread_create(&thread,NULL,sampler,NULL)) {
        __atomic_store_n(&requested,0,__ATOMIC_RELEASE);
        pthread_mutex_unlock(&state_lock);return 0;
    }
    pthread_detach(thread);
    started=1;
    pthread_mutex_unlock(&state_lock);
    return 1;
}

int dexcore_overlay_set_enabled(int enabled)
{
    if(!dexcore_overlay_start())return 0;
    enabled=!!enabled;
    pthread_mutex_lock(&state_lock);
    FILE *file=fopen(DEXCORE_OVERLAY_TEMP,"wb");
    int ok=0;
    if(file) {
        ok=fputc(enabled?'1':'0',file)!=EOF;
        ok=(fputc('\n',file)!=EOF) && ok;
        if(fflush(file))ok=0;
        if(fclose(file))ok=0;
        if(ok && rename(DEXCORE_OVERLAY_TEMP,DEXCORE_OVERLAY_STATE))ok=0;
    }
    if(ok) {
        __atomic_add_fetch(&epoch,1,__ATOMIC_ACQ_REL);
        __atomic_store_n(&requested,enabled,__ATOMIC_RELEASE);
        frame.enabled=enabled;frame.ready=0;++frame.revision;
        memset(&frame.metrics,0,sizeof(frame.metrics));
        snprintf(frame.text,sizeof(frame.text),"DEXCORE | System Information\nCollecting sensor readings...");
    } else if(file)remove(DEXCORE_OVERLAY_TEMP);
    pthread_mutex_unlock(&state_lock);
    return ok;
}

int dexcore_overlay_get_frame(DexcoreOverlayFrame *out)
{
    if(!out || pthread_mutex_trylock(&state_lock))return 0;
    *out=frame;
    pthread_mutex_unlock(&state_lock);
    return 1;
}
