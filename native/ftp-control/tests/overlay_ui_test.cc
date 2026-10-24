/* Execute the actual renderer against a deterministic Mono/UI2 facade.
 * Checks ownership, scene changes and exception paths; not PS4 visual QA.
 */
#define DEXCORE_OVERLAY_UI_TEST
#include "../plugin_mono/source/system_overlay_ui.cc"
#include <assert.h>
#include <stdlib.h>

struct Class {const char *name;Class *parent;};
struct Object {Class *klass;Object *parent,*reference;Object *children[8];unsigned count;int initialized;char text[1024];int visible;};
struct Method {Class *klass;char name[64];int setter;};
static Class widget={"Widget",nullptr},panel={"Panel",&widget},label={"Label",&widget};
static Class scene={"Scene",nullptr},font={"UIFont",nullptr},color={"UIColor",nullptr};
static Class area_type={"AreaManager",nullptr},layer={"LayerManager",nullptr},string_type={"String",nullptr};
static Object game_root={&widget},top_root={&widget},next_root={&widget};
static Object game_scene={&scene},top_scene={&scene},area_object={&area_type};
static Object *handles[2048];static unsigned handle_next=1,live_handles;
static Method methods[1024];static unsigned method_next;
static DexcoreOverlayFrame input;
static uint64_t clock_us=1000000;
static int text_updates,reject_text,reject_remove,missing_panel;
static pthread_t ui_thread;

extern "C" {
void *Root_Domain=(void*)1,*App_Exe=(void*)2,*MSCorlib=(void*)3;
uint64_t sceKernelGetProcessTime(void){return clock_us;}
char *sceKernelGetFsSandboxRandomWord(void){return (char*)"sandbox";}
int dexcore_overlay_enabled(void){return input.enabled;}
int dexcore_overlay_get_frame(DexcoreOverlayFrame *out){*out=input;return 1;}
void *mono_get_image(const char*){return (void*)4;}
void *mono_class_from_name(const void*,const char*,const char *name)
{
    Class *classes[]={&widget,&panel,&label,&scene,&font,&color,&area_type,&layer};
    for(unsigned i=0;i<sizeof(classes)/sizeof(classes[0]);i++)
        if(!strcmp(name,classes[i]->name))return missing_panel && classes[i]==&panel?nullptr:classes[i];
    return nullptr;
}
void *mono_class_get_parent(const void *klass){return ((Class*)klass)->parent;}
void *mono_class_get_method_from_name(const void *klass,const char *name,int)
{
    assert(method_next<1024);Method *m=&methods[method_next++];m->klass=(Class*)klass;
    snprintf(m->name,sizeof(m->name),"%s",name);return m;
}
void *mono_class_get_property_from_name(const void *klass,const char *name)
{
    return mono_class_get_method_from_name(klass,name,0);
}
void *mono_property_get_get_method(const void *prop){return (void*)prop;}
void *mono_property_get_set_method(const void *prop){((Method*)prop)->setter=1;return (void*)prop;}
void *mono_object_new(const void*,const void *klass)
{
    Object *object=(Object*)calloc(1,sizeof(Object));assert(object);object->klass=(Class*)klass;return object;
}
int mono_class_is_valuetype(const void *klass){return klass==&font || klass==&color;}
void *mono_object_unbox(const void *object){return (void*)object;}
uint32_t mono_gchandle_new(void *object,int pinned)
{
    assert(pinned==1);assert(handle_next<2048);handles[handle_next]=((Object*)object);++live_handles;return handle_next++;
}
void *mono_gchandle_get_target(uint32_t handle){assert(handle<2048);return handles[handle];}
void mono_gchandle_free(uint32_t handle){assert(handles[handle]);handles[handle]=nullptr;--live_handles;}
MonoString *Mono_New_String(const char *text)
{
    Object *object=(Object*)mono_object_new(nullptr,&string_type);snprintf(object->text,sizeof(object->text),"%s",text);
    return (MonoString*)object;
}
void *mono_runtime_invoke(const void *method_ptr,const void *instance,const void *args[],const void *exc[])
{
    assert(pthread_equal(pthread_self(),ui_thread));Method *m=(Method*)method_ptr;Object *object=(Object*)instance;
    if(m->setter) {
        if(!strcmp(m->name,"Visible"))object->visible=*(const bool*)args[0];
        if(!strcmp(m->name,"Text")) {
            if(reject_text){*exc=(void*)99;return nullptr;}
            assert(object->initialized);assert(((Object*)args[0])->klass==&string_type);
            snprintf(object->text,sizeof(object->text),"%s",((Object*)args[0])->text);++text_updates;
        }
        return nullptr;
    }
    if(!strcmp(m->name,".ctor")){assert(object);object->initialized=1;return nullptr;}
    if(!strcmp(m->name,"AppendChild")) {
        Object *child=(Object*)args[0];assert(child && child->initialized && !child->parent);
        assert(object->count<8);object->children[object->count++]=child;child->parent=object;return nullptr;
    }
    if(!strcmp(m->name,"RemoveFromParent")) {
        if(reject_remove){*exc=(void*)99;return nullptr;}
        if(object->parent) {
            Object *parent=object->parent;
            for(unsigned i=0;i<parent->count;i++)if(parent->children[i]==object) {
                for(unsigned j=i+1;j<parent->count;j++)parent->children[j-1]=parent->children[j];
                --parent->count;break;
            }
            object->parent=nullptr;
        }
        return nullptr;
    }
    if(!strcmp(m->name,"Instance"))return &area_object;
    if(!strcmp(m->name,"GetTopScene"))return &top_scene;
    if(!strcmp(m->name,"FindContainerSceneByPath")){assert(!strcmp(((Object*)args[0])->text,"Game"));return &game_scene;}
    if(!strcmp(m->name,"RootWidget"))return object->reference;
    assert(!"unexpected managed UI call");return nullptr;
}
}

static void step(void){clock_us+=1000000;render_tick();}
int main(void)
{
    ui_thread=pthread_self();ui_image=(void*)4;base_namespace="Sce.PlayStation.PUI";ui_namespace="Sce.PlayStation.PUI.UI2";
    game_scene.reference=&game_root;top_scene.reference=&top_root;
    input.enabled=1;input.ready=1;input.revision=1;snprintf(input.text,sizeof(input.text),"CPU: 62 C\nFan: 50%%");
    missing_panel=1;step();assert(!game_root.count && !top_root.count && !live_handles);
    missing_panel=0;step();assert(game_root.count==1 && top_root.count==1);assert(live_handles==7);
    assert(strstr(game_root.children[0]->children[0]->text,"62 C"));assert(text_updates==2);
    step();assert(text_updates==2 && live_handles==7);
    ++input.revision;snprintf(input.text,sizeof(input.text),"CPU: 63 C\nFan: 51%%");step();assert(text_updates==4);
    top_scene.reference=&next_root;step();assert(!top_root.count && next_root.count==1 && live_handles==7);
    top_scene.reference=&game_root;step();assert(!next_root.count && game_root.count==1 && live_handles==4);
    input.enabled=0;reject_remove=1;render_tick();
    assert(game_root.count==1 && !game_root.children[0]->visible && live_handles==4);
    reject_remove=0;render_tick();assert(!game_root.count && live_handles==1);
    input.enabled=1;++input.revision;step();assert(game_root.count==1 && live_handles==4);
    reject_text=1;++input.revision;step();assert(!game_root.count && live_handles==1);
    for(int i=0;i<3;i++){step();assert(!game_root.count && live_handles==1);}
    reject_text=0;step();assert(game_root.count==1 && live_handles==4);
    input.enabled=0;render_tick();assert(!game_root.count && live_handles==1);
    puts("PASS: actual renderer attaches to Game/ShellUI roots, updates text, survives scene changes, avoids duplicate roots, frees panel handles on Off and retries caught UI exceptions");
    return 0;
}
