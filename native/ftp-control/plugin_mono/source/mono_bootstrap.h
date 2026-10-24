/* DEXCORE Mono startup retry, GPL-3.0-or-later. */
#ifndef DEXCORE_MONO_BOOTSTRAP_H
#define DEXCORE_MONO_BOOTSTRAP_H
#ifdef __cplusplus
extern "C" {
#endif
void *dexcore_mono_loaded_image(const char *name);
int dexcore_mono_bootstrap_step(void);
int dexcore_mono_bootstrap_start(void);
#ifdef __cplusplus
}
#endif
#endif
