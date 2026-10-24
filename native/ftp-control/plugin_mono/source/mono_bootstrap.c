/* DEXCORE: do not depend on another module loading after app.exe.
 * Enumerate already-loaded assemblies and retry while Mono finishes startup.
 * Metadata/hook setup can run on an attached thread; widget operations cannot.
 * GPL-3.0-or-later.
 */
#include "mono.h"
#include "mono_bootstrap.h"
#include "system_overlay.h"
#include <pthread.h>
#include <string.h>
#include <unistd.h>
extern void mono_assembly_foreach(void (*callback)(void *,void *),void *data);
extern const char *mono_image_get_name(const void *);
extern void UploadOnBranch(void *);
extern int DexcoreSettingsHooksReady(void);
extern void *mono_thread_current(void);
extern void mono_thread_detach(void *);

static pthread_mutex_t setup_lock=PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t start_lock=PTHREAD_MUTEX_INITIALIZER;
static int started;
struct ImageSearch {const char *name;void *image;};
static int image_name_equal(const char *name,const char *expected)
{
    const char *slash=strrchr(name,'/');if(slash)name=slash+1;
    size_t n=strlen(name),e=strlen(expected);
    if(n!=e && !(e>4 && n==e-4))return 0;
    for(size_t i=0;i<n;i++) {
        char a=name[i],b=expected[i];
        if(a>='A' && a<='Z')a+='a'-'A';
        if(b>='A' && b<='Z')b+='a'-'A';
        if(a!=b)return 0;
    }
    return 1;
}
static void find_image(void *assembly,void *data)
{
    struct ImageSearch *search=data;
    if(search->image)return;
    void *image=mono_assembly_get_image(assembly);
    const char *name=image?mono_image_get_name(image):NULL;
    if(name && image_name_equal(name,search->name))search->image=image;
}
void *dexcore_mono_loaded_image(const char *name)
{
    struct ImageSearch search={name,NULL};
    mono_assembly_foreach(find_image,&search);
    return search.image;
}
int dexcore_mono_bootstrap_step(void)
{
    if(pthread_mutex_trylock(&setup_lock))return 0;
    if(!Root_Domain)Root_Domain=mono_get_root_domain();
    if(!Root_Domain){pthread_mutex_unlock(&setup_lock);return 0;}
    mono_thread_attach(Root_Domain);
    if(!App_Exe)App_Exe=dexcore_mono_loaded_image("app.exe");
    if(App_Exe && !DexcoreSettingsHooksReady()) {
        void *klass=mono_class_from_name(App_Exe,"Sce.Vsh.ShellUI.Settings.SettingsRoot","SettingsRootHandler");
        if(klass && mono_class_get_method_from_name(klass,"OnPress",2) &&
                    mono_class_get_method_from_name(klass,"OnPreCreate",2))
            UploadOnBranch(App_Exe);
    }
    if(App_Exe)dexcore_overlay_ui_install();
    int ready=DexcoreSettingsHooksReady() && dexcore_overlay_ui_installed();
    pthread_mutex_unlock(&setup_lock);
    return ready;
}
static void *bootstrap(void *unused)
{
    (void)unused;
    while(!dexcore_mono_bootstrap_step())usleep(500000);
    mono_thread_detach(mono_thread_current());
    return NULL;
}
int dexcore_mono_bootstrap_start(void)
{
    pthread_mutex_lock(&start_lock);
    if(started){pthread_mutex_unlock(&start_lock);return 1;}
    pthread_t thread;
    int ok=pthread_create(&thread,NULL,bootstrap,NULL)==0;
    if(ok){pthread_detach(thread);started=1;}
    pthread_mutex_unlock(&start_lock);return ok;
}
