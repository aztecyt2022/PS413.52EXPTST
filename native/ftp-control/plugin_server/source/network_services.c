/* DEXCORE 1.1 / Native BinLoader + Klog services; GPL-3.0-or-later.
 * Both services are explicitly enabled via persisted PS4 Settings state files.
 * This runs inside the existing native plugin_server.prx; not in the website.
 * Note: the raw .bin entrypoint interface is EXPERIMENTAL and only supports
 * payloads compatible with in-process PIC execution and no implicit args.
 */
#define _DEFAULT_SOURCE
#include "network_services.h"
#include <stdio.h>
#ifndef DEXCORE_NETWORK_TEST
#include "../../common/notify.h"
#else
#include <stdarg.h>
static void dexcore_test_notify(const char* icon, const char* fmt, ...);
#define TEX_ICON_SYSTEM "SYSTEM"
#define Notify dexcore_test_notify
#endif
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#ifndef DEXCORE_NETWORK_STATE_DIR
#define DEXCORE_NETWORK_STATE_DIR "/data/hen"
#endif
#ifndef DEXCORE_KLOG_SOURCE
#define DEXCORE_KLOG_SOURCE "/dev/klog"
#endif
#ifndef DEXCORE_KLOG_PORT
#define DEXCORE_KLOG_PORT 3232
#endif
#ifndef DEXCORE_BINLOADER_PORT
#define DEXCORE_BINLOADER_PORT 9090
#endif
#define DEXCORE_MAX_PAYLOAD (2u * 1024u * 1024u)
#define DEXCORE_KLOG_CHUNK 1024

#ifdef DEXCORE_NETWORK_TEST
static void dexcore_test_notify(const char* icon, const char* fmt, ...)
{
    (void)icon;
    va_list ap;
    va_start(ap, fmt);
    fputs("PS4 NOTIFY: ", stderr);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
}
#endif

/* Service names are fixed literals, never accepted from network input. */
int dexcore_service_enabled(const char* name)
{
    if (strcmp(name, "binloader_enabled") && strcmp(name, "klog_enabled"))
        return 0;
    char path[128];
    if (snprintf(path, sizeof(path), "%s/%s", DEXCORE_NETWORK_STATE_DIR, name) >= (int)sizeof(path))
        return 0;
    FILE* f = fopen(path, "rb");
    if (!f) return 0; /* Absent or inaccessible state: disabled. */
    int value = fgetc(f);
    fclose(f);
    return value == '1';
}

static void local_ip(char out[INET_ADDRSTRLEN])
{
    strcpy(out, "0.0.0.0");
    struct ifaddrs* all = NULL;
    if (getifaddrs(&all)) return;
    for (struct ifaddrs* p = all; p; p = p->ifa_next)
    {
        if (!p->ifa_addr || p->ifa_addr->sa_family != AF_INET ||
            !strncmp(p->ifa_name, "lo", 2)) continue;
        struct sockaddr_in* addr = (struct sockaddr_in*)p->ifa_addr;
        char ip[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip)) &&
            strncmp(ip, "0.", 2) && strncmp(ip, "127.", 4))
        {
            snprintf(out, INET_ADDRSTRLEN, "%s", ip);
            break;
        }
    }
    freeifaddrs(all);
}

static int make_listener(unsigned port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    int one = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)port);
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) || listen(fd, 3))
    {
        close(fd);
        return -1;
    }
    return fd;
}

static int wait_connection(int server_fd, const char* state)
{
    fd_set r;
    FD_ZERO(&r);
    FD_SET(server_fd, &r);
    struct timeval tv = {0, 250000};
    int result = select(server_fd + 1, &r, NULL, NULL, &tv);
    if (result < 0 && errno == EINTR) return 0;
    if (!dexcore_service_enabled(state)) return 0;
    return result > 0 && FD_ISSET(server_fd, &r);
}

/* No peer endpoint is logged or shown. Keep only generic event notification. */
static void peer_notification(const char* service)
{
    Notify(TEX_ICON_SYSTEM, "DEXCORE %s client connected", service);
}

static int receive_klog(int fd, const char* state)
{
    int logfd = open(DEXCORE_KLOG_SOURCE, O_RDONLY | O_NONBLOCK);
    if (logfd < 0) return -1;
    char buf[DEXCORE_KLOG_CHUNK];
    int failed = 0;
    while (dexcore_service_enabled(state))
    {
        /* Detect peer hangup rather than holding onto abandoned connections. */
        char probe;
        ssize_t p = recv(fd, &probe, 1, MSG_PEEK | MSG_DONTWAIT);
        if (p == 0 || (p < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR))
            break;
        ssize_t n = read(logfd, buf, sizeof(buf));
        if (n > 0)
        {
            size_t sent = 0;
            while (sent < (size_t)n && dexcore_service_enabled(state))
            {
                ssize_t w = send(fd, buf + sent, (size_t)n - sent, 0);
                if (w <= 0) { failed = 1; break; }
                sent += (size_t)w;
            }
            if (failed) break;
        }
        else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
            break;
        usleep(30000);
    }
    close(logfd);
    return failed ? -1 : 0;
}

static void* klog_main(void* unused)
{
    (void)unused;
    const char* state = "klog_enabled";
    for (;;)
    {
        if (!dexcore_service_enabled(state)) { usleep(250000); continue; }
        int listener = make_listener(DEXCORE_KLOG_PORT);
        if (listener < 0)
        {
            Notify(TEX_ICON_SYSTEM, "DEXCORE Klog: port 3232 unavailable");
            sleep(2);
            continue;
        }
        char host[INET_ADDRSTRLEN]; local_ip(host);
        Notify(TEX_ICON_SYSTEM, "DEXCORE Klog enabled\n%s:%u", host, DEXCORE_KLOG_PORT);
        while (dexcore_service_enabled(state))
        {
            if (!wait_connection(listener, state)) continue;
            /* The client address is intentionally not retrieved. */
            int client = accept(listener, NULL, NULL);
            if (client < 0) continue;
            if (!dexcore_service_enabled(state)) { close(client); break; }
            peer_notification("Klog");
#ifdef SO_NOSIGPIPE
            int one=1; setsockopt(client,SOL_SOCKET,SO_NOSIGPIPE,&one,sizeof(one));
#endif
            struct timeval tv={1,0};
            setsockopt(client,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(tv));
            if (receive_klog(client, state) < 0)
                Notify(TEX_ICON_SYSTEM, "DEXCORE Klog: client disconnected");
            close(client);
        }
        close(listener);
        Notify(TEX_ICON_SYSTEM, "DEXCORE Klog disabled (TCP 3232)");
    }
    return NULL;
}

/* The BinLoader protocol is unframed: one raw position-independent .bin
 * payload per TCP connection, ending in EOF. It deliberately rejects ELF,
 * PRX, unbounded streams and incomplete/busy sessions. The sender MUST close
 * its write half after sending; remote code executes with plugin privileges.
 */
struct dexcore_payload_thread_args {void* memory; size_t size;};
static pthread_mutex_t bin_busy_lock = PTHREAD_MUTEX_INITIALIZER;
static int bin_busy = 0;
static void* run_payload(void* ptr)
{
    struct dexcore_payload_thread_args* a = (struct dexcore_payload_thread_args*)ptr;
    /* Native payload ABI varies. This supports only the entry(void) flat ABI. */
    int (*entry)(void) = (int (*)(void))a->memory;
    entry();
    munmap(a->memory, a->size);
    free(a);
    pthread_mutex_lock(&bin_busy_lock);
    bin_busy = 0;
    pthread_mutex_unlock(&bin_busy_lock);
    return NULL;
}
static int load_binary(int client, const char* state)
{
    /* Do not block the supervisor forever if sender stalls. */
    struct timeval timeout = {5, 0};
    setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    void* payload = mmap(NULL, DEXCORE_MAX_PAYLOAD,
                         PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
    if (payload == MAP_FAILED) return -1;
    size_t total = 0;
    char* dst = (char*)payload;
    int rejected = 0;
    while (dexcore_service_enabled(state) && total < DEXCORE_MAX_PAYLOAD)
    {
        ssize_t n = recv(client, dst + total, DEXCORE_MAX_PAYLOAD - total, 0);
        if (n == 0) break;
        if (n < 0) { rejected = 1; break; }
        total += (size_t)n;
    }
    if (!dexcore_service_enabled(state) || !total || total == DEXCORE_MAX_PAYLOAD ||
        (unsigned char)dst[0] != 0xE9 || rejected)
    {
        munmap(payload, DEXCORE_MAX_PAYLOAD);
        Notify(TEX_ICON_SYSTEM, "DEXCORE BinLoader rejected payload");
        return -1;
    }
    if (mprotect(payload, DEXCORE_MAX_PAYLOAD, PROT_READ|PROT_EXEC) != 0)
    {
        munmap(payload, DEXCORE_MAX_PAYLOAD);
        Notify(TEX_ICON_SYSTEM, "DEXCORE BinLoader executable mapping failed");
        return -1;
    }
    struct dexcore_payload_thread_args* args = malloc(sizeof(*args));
    if (!args) {munmap(payload,DEXCORE_MAX_PAYLOAD);return -1;}
    args->memory=payload;
    args->size=DEXCORE_MAX_PAYLOAD;
    pthread_t worker;
    if (pthread_create(&worker, NULL, run_payload, args))
    {
        munmap(payload, DEXCORE_MAX_PAYLOAD);
        free(args);
        return -1;
    }
    pthread_detach(worker);
    Notify(TEX_ICON_SYSTEM, "DEXCORE BinLoader launched %u bytes", (unsigned)total);
    return 0;
}

static void* binloader_main(void* unused)
{
    (void)unused;
    const char* state = "binloader_enabled";
    for (;;)
    {
        if (!dexcore_service_enabled(state)) { usleep(250000); continue; }
        int listener=make_listener(DEXCORE_BINLOADER_PORT);
        if (listener < 0)
        {
            Notify(TEX_ICON_SYSTEM, "DEXCORE BinLoader: port 9090 unavailable");
            sleep(2);
            continue;
        }
        char host[INET_ADDRSTRLEN];local_ip(host);
        Notify(TEX_ICON_SYSTEM,"DEXCORE BinLoader enabled\n%s:%u",host,DEXCORE_BINLOADER_PORT);
        while(dexcore_service_enabled(state))
        {
            if(!wait_connection(listener,state)) continue;
            /* The client address is intentionally not retrieved. */
            int client=accept(listener,NULL,NULL);
            if(client<0) continue;
            if(!dexcore_service_enabled(state)) {close(client);break;}
            peer_notification("BinLoader");
            pthread_mutex_lock(&bin_busy_lock);
            if(bin_busy)
            {
                pthread_mutex_unlock(&bin_busy_lock);
                Notify(TEX_ICON_SYSTEM,"DEXCORE BinLoader: payload still running");
                close(client);
                continue;
            }
            bin_busy=1;
            pthread_mutex_unlock(&bin_busy_lock);
            if(load_binary(client,state)<0)
            {
                pthread_mutex_lock(&bin_busy_lock);
                bin_busy=0;
                pthread_mutex_unlock(&bin_busy_lock);
            }
            close(client);
        }
        close(listener);
        Notify(TEX_ICON_SYSTEM, "DEXCORE BinLoader disabled (TCP 9090)");
    }
    return NULL;
}

void* dexcore_klog_supervisor(void* arg) { return klog_main(arg); }
void* dexcore_binloader_supervisor(void* arg) { return binloader_main(arg); }
