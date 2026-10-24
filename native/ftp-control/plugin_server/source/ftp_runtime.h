/* DEXCORE native FTP lifecycle: GPL-3.0-or-later.
 * Applies to Scene-Collective ps4-hen-plugins/plugin_server.
 */
#ifndef DEXCORE_FTP_RUNTIME_H
#define DEXCORE_FTP_RUNTIME_H
void* pthread_ftp_supervisor(void* unused);
int ftp_is_active(void);
void ftp_shutdown_clients(void);
#endif
