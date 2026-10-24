/* DEXCORE 1.1 / Native network services (GPL-3.0-or-later) */
#ifndef DEXCORE_NETWORK_SERVICES_H
#define DEXCORE_NETWORK_SERVICES_H

void* dexcore_klog_supervisor(void* arg);
void* dexcore_binloader_supervisor(void* arg);
int dexcore_service_enabled(const char* name);

#endif
