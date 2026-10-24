/* Linux behavioral smoke test of the SAME FTP supervisor source used on PS4.
 * It replaces only pthread_ftp and client shutdown with harmless test stubs.
 * Does not imply the PS4 PRX has been built or tested on a console.
 */
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ftp_runtime.h"

#define CONTROL "/tmp/dexcore-native-ftp-ci-state"
static volatile int launched = 0;
static volatile int exited = 0;
static volatile int shutdown_calls = 0;

void ftp_shutdown_clients(void) {
    __atomic_add_fetch(&shutdown_calls, 1, __ATOMIC_SEQ_CST);
}
void* pthread_ftp(void* unused) {
    (void)unused;
    __atomic_add_fetch(&launched, 1, __ATOMIC_SEQ_CST);
    while(ftp_is_active()) usleep(10000);
    __atomic_add_fetch(&exited, 1, __ATOMIC_SEQ_CST);
    return NULL;
}
static void set_enabled(char mode) {
    char temp[] = CONTROL ".tmp";
    FILE* f=fopen(temp,"wb");
    assert(f);
    assert(fputc(mode,f) != EOF);
    assert(fclose(f)==0);
    assert(rename(temp,CONTROL)==0);
}
static void wait_until(volatile int* target,int expected) {
    for(int i=0;i<100;i++){
        if(__atomic_load_n(target,__ATOMIC_SEQ_CST)>=expected)return;
        usleep(20000);
    }
    fprintf(stderr,"Timed out: got %d, expected %d\n",
            __atomic_load_n(target,__ATOMIC_SEQ_CST),expected);
    abort();
}
int main(void) {
    unlink(CONTROL);
    pthread_t supervisor;
    assert(pthread_create(&supervisor,NULL,pthread_ftp_supervisor,NULL)==0);
    usleep(400000);
    assert(launched==0); /* OFF by default. */
    set_enabled('1');
    wait_until(&launched,1);
    assert(ftp_is_active());
    set_enabled('0');
    wait_until(&exited,1);
    wait_until(&shutdown_calls,1);
    assert(!ftp_is_active());
    set_enabled('1');
    wait_until(&launched,2); /* Can restart after boot. */
    set_enabled('0');
    wait_until(&exited,2);
    pthread_cancel(supervisor);
    pthread_join(supervisor,NULL);
    unlink(CONTROL);
    puts("PASS: native FTP is default-off, independently starts/stops, and can restart.");
    return 0;
}
