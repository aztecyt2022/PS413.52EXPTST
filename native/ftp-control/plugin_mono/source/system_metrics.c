/* DEXCORE - read-only native PS4 health snapshots (GPL-3.0-or-later).
 * Use native libkernel exports if present. Never assume sensor availability.
 * Memory estimate uses hw.physmem - (free pages x pagesize). Page cache and
 * other reclaimable memory may be counted as "used"; label ESTIMATED.
 */
#include "system_metrics.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#ifdef DEXCORE_METRICS_TEST
/* Portable stubs exercise actual source logic and failures. */
extern int dexcore_test_cpu(int *out);
extern int dexcore_test_soc(int sensor, int *out);
extern int dexcore_test_fan(uint16_t *duty, uint64_t *chassis);
extern int dexcore_test_sysctl(const char *name, void *out, size_t *len);
#else
#include <orbis/libkernel.h>
/* sceKernelDlsym is used for version-dependent kernel APIs so unsupported
 * methods yield Unavailable rather than a crash / false reading. */
static void *resolve_kernel(const char *symbol)
{
    void *address = NULL;
    if (sceKernelDlsym(0x2001, symbol, &address) != 0) return NULL;
    return address;
}
#endif
static int kernel_cpu(int *celsius)
{
#ifdef DEXCORE_METRICS_TEST
    return dexcore_test_cpu(celsius);
#else
    typedef int (*fn_t)(int *);
    fn_t fn = (fn_t)resolve_kernel("sceKernelGetCpuTemperature");
    return fn ? fn(celsius) : -1;
#endif
}
static int kernel_soc(int sensor, int *celsius)
{
#ifdef DEXCORE_METRICS_TEST
    return dexcore_test_soc(sensor,celsius);
#else
    typedef int (*fn_t)(int, int *);
    fn_t fn=(fn_t)resolve_kernel("sceKernelGetSocSensorTemperature");
    return fn ? fn(sensor,celsius) : -1;
#endif
}
static int kernel_fan(uint16_t *duty, uint64_t *chassis)
{
#ifdef DEXCORE_METRICS_TEST
    return dexcore_test_fan(duty,chassis);
#else
    typedef int (*fn_t)(uint16_t *,uint64_t *);
    fn_t fn=(fn_t)resolve_kernel("sceKernelGetCurrentFanDuty");
    return fn ? fn(duty,chassis) : -1;
#endif
}
static int get_sysctl_u64(const char *name, uint64_t *value)
{
    size_t size=sizeof(*value);
    uint64_t tmp=0;
#ifdef DEXCORE_METRICS_TEST
    int rc=dexcore_test_sysctl(name,&tmp,&size);
#else
    typedef int (*fn_t)(const char *, void *, size_t *, const void *, size_t);
    fn_t fn=(fn_t)resolve_kernel("sysctlbyname");
    int rc=fn ? fn(name,&tmp,&size,NULL,0) : -1;
#endif
    if (rc != 0 || (size != sizeof(uint64_t) && size != sizeof(uint32_t))) return -1;
    *value=tmp;
    return 0;
}
void dexcore_metrics_read(DexcoreMetrics *out)
{
    if (!out) return;
    memset(out,0,sizeof(*out));
    int degrees=0;
    if (kernel_cpu(&degrees)==0 && degrees>-20 && degrees<135) {
        out->cpu_valid=1;out->cpu_celsius=degrees;
    }
    /* Some systems expose several numbered SoC sensors; take the warmest
     * returned value, without assuming every PS4 model provides all IDs. */
    for(int id=0;id<8;id++) {
        degrees=0;
        if(kernel_soc(id,&degrees)==0 && degrees>-20 && degrees<135){
            if(!out->soc_valid || degrees>out->soc_celsius)
                out->soc_celsius=degrees;
            out->soc_valid=1;
            if(id==0){out->soc0_valid=1;out->soc0_celsius=degrees;}
            if(id==1){out->soc1_valid=1;out->soc1_celsius=degrees;}
        }
    }
    uint16_t duty=0;
    uint64_t chassis=0;
    /* Q10 fan duty: 0..1024 => 0..100%. Chassis output is NOT fan RPM. */
    if(kernel_fan(&duty,&chassis)==0 && duty<=1024) {
        out->fan_valid=1;
        out->fan_percent=(int)(((uint32_t)duty*100+512)/1024);
    }
    uint64_t total=0,free_pages=0,page_size=0;
    if(get_sysctl_u64("hw.physmem",&total)==0 &&
       get_sysctl_u64("vm.stats.vm.v_free_count",&free_pages)==0 &&
       get_sysctl_u64("hw.pagesize",&page_size)==0 &&
       total && page_size && page_size<=65536 &&
       free_pages<=total/page_size)
    {
        uint64_t free_bytes=free_pages*page_size;
        out->ram_valid=1;
        out->total_bytes=total;
        out->used_bytes=total-free_bytes;
        out->ram_percent=(int)((out->used_bytes*100)/total);
    }
}
void dexcore_metrics_format(const DexcoreMetrics *m,const char *id,
                            char *out,unsigned outlen)
{
    if (!out||!outlen) return;
    out[0]='\0';
    if (!m||!id) return;
    if(strcmp(id,"system_info_cpu")==0) {
        if(m->cpu_valid) snprintf(out,outlen,"CPU / APU: %d C",m->cpu_celsius);
        else snprintf(out,outlen,"CPU / APU: Unavailable");
    } else if(strcmp(id,"system_info_soc0")==0) {
        if(m->soc0_valid) snprintf(out,outlen,"SoC Sensor 0: %d C",m->soc0_celsius);
        else snprintf(out,outlen,"SoC Sensor 0: Unavailable");
    } else if(strcmp(id,"system_info_soc1")==0) {
        if(m->soc1_valid) snprintf(out,outlen,"SoC Sensor 1: %d C",m->soc1_celsius);
        else snprintf(out,outlen,"SoC Sensor 1: Unavailable");
    } else if(strcmp(id,"system_info_soc")==0) {
        if(m->soc_valid) snprintf(out,outlen,"SoC Sensor (Max): %d C",m->soc_celsius);
        else snprintf(out,outlen,"SoC Sensor: Unavailable");
    } else if(strcmp(id,"system_info_fan")==0) {
        if(m->fan_valid) snprintf(out,outlen,"Fan Duty: %d%%",m->fan_percent);
        else snprintf(out,outlen,"Fan Duty: Unavailable");
    } else if(strcmp(id,"system_info_ram")==0) {
        if(m->ram_valid) snprintf(out,outlen,"RAM Used (Est.): %d%% (%llu / %llu MiB)",
            m->ram_percent,(unsigned long long)(m->used_bytes/1048576),
            (unsigned long long)(m->total_bytes/1048576));
        else snprintf(out,outlen,"RAM Used: Unavailable");
    }
}
