/* DEXCORE - native PS4 System Information read-only metrics (GPL-3.0-or-later).
 * These functions DO NOT modify fan settings, clocks, kernel or sysctl values.
 */
#ifndef DEXCORE_SYSTEM_METRICS_H
#define DEXCORE_SYSTEM_METRICS_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct DexcoreMetrics {
    int cpu_valid, cpu_celsius;
    int soc_valid, soc_celsius;
    int soc0_valid, soc0_celsius;
    int soc1_valid, soc1_celsius;
    int fan_valid, fan_percent;
    int ram_valid, ram_percent;
    uint64_t total_bytes, used_bytes;
} DexcoreMetrics;
void dexcore_metrics_read(DexcoreMetrics *out);
void dexcore_metrics_format(const DexcoreMetrics *m, const char *id,
                            char *out, unsigned outlen);
#ifdef __cplusplus
}
#endif
#endif
