/* Actual startup coordinator: no later native module-load event is required. */
#include "mono.h"
#include "mono_bootstrap.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
void *Root_Domain,*App_Exe,*MSCorlib;
static int domain_ready,app_loaded,ui_loaded,settings_ready,renderer_ready,installs,attached;
void *mono_get_root_domain(void){return domain_ready?(void*)1:NULL;}
void mono_thread_attach(void *domain){assert(domain==(void*)1);attached=1;}
void *mono_thread_current(void){return (void*)1;}
void mono_thread_detach(void *thread){assert(thread==(void*)1);}
void mono_assembly_foreach(void (*fn)(void*,void*),void *data)
{
    assert(attached);
    if(app_loaded)fn((void*)2,data);
    if(ui_loaded)fn((void*)3,data);
}
void *mono_assembly_get_image(const void *assembly){return (void*)assembly;}
const char *mono_image_get_name(const void *image)
{
    return image==(void*)2?"/app0/psm/Application/app.exe":"Sce.PlayStation.PUI.dll";
}
void *mono_class_from_name(const void *image,const char *ns,const char *name)
{
    assert(image==(void*)2);assert(!strcmp(ns,"Sce.Vsh.ShellUI.Settings.SettingsRoot"));
    assert(!strcmp(name,"SettingsRootHandler"));return (void*)4;
}
void *mono_class_get_method_from_name(const void *klass,const char *name,int count)
{
    assert(klass==(void*)4 && count==2);assert(!strcmp(name,"OnPress") || !strcmp(name,"OnPreCreate"));return (void*)5;
}
void UploadOnBranch(void *image){assert(image==(void*)2);++installs;settings_ready=1;}
int DexcoreSettingsHooksReady(void){return settings_ready;}
void dexcore_overlay_ui_install(void){if(ui_loaded)renderer_ready=1;}
int dexcore_overlay_ui_installed(void){return renderer_ready;}
int main(void)
{
    assert(!dexcore_mono_bootstrap_step());assert(!installs);
    domain_ready=1;assert(!dexcore_mono_bootstrap_step());assert(!installs);
    app_loaded=1;assert(!dexcore_mono_bootstrap_step());assert(installs==1 && settings_ready);
    ui_loaded=1;assert(dexcore_mono_bootstrap_step());assert(renderer_ready && installs==1);
    assert(dexcore_mono_bootstrap_step() && installs==1);
    puts("PASS: delayed Mono/app/UI startup retries to completion without another module-load event or duplicate Settings installation");
    return 0;
}
