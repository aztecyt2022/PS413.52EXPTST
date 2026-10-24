/* DEXCORE native FTP supervisor: GPL-3.0-or-later.
 * FTP is OFF if the state file does not exist or contains anything but 1.
 * The native Settings plugin writes the state file atomically.
 * The supervisor controls only FTP; klog stays in its original thread.
 */
#include "ftp_runtime.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#ifndef DEXCORE_FTP_CONTROL_FILE
#define DEXCORE_FTP_CONTROL_FILE "/data/hen/ftp_enabled"
#endif
static volatile int g_ftp_active = 0;

extern void* pthread_ftp(void* args);

int ftp_is_active(void)
{
    return __atomic_load_n(&g_ftp_active, __ATOMIC_ACQUIRE);
}

static void set_active(int active)
{
    __atomic_store_n(&g_ftp_active, active, __ATOMIC_RELEASE);
}

static int read_requested(void)
{
    FILE* f = fopen(DEXCORE_FTP_CONTROL_FILE, "rb");
    if (!f) return 0;
    int state = fgetc(f);
    fclose(f);
    return state == '1';
}

void* pthread_ftp_supervisor(void* unused)
{
    (void)unused;
    pthread_t ftp_worker;
    int started = 0;
    set_active(0);
    for (;;)
    {
        int requested = read_requested();
        if (requested && !started)
        {
            set_active(1);
            if (pthread_create(&ftp_worker, NULL, pthread_ftp, NULL) == 0)
                started = 1;
            else
                set_active(0);
        }
        else if (!requested && started)
        {
            set_active(0);
            ftp_shutdown_clients();
            pthread_join(ftp_worker, NULL);
            started = 0;
        }
        else if (!requested)
            set_active(0);

        usleep(250000);
    }
    return NULL;
}
