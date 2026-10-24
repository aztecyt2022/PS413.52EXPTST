/* DEXCORE persistent native health panel. GPL-3.0-or-later.
 * Runtime-resolved Mono UI2 widgets; no firmware UI addresses are invented.
 * UI2 class/property and Game-layer attachment conventions documented by
 * OSM-Made/Orbis-Toolbox (Game_Overlay.cpp, UI_Utilities.cpp, Label.cpp).
 * All widget creation, reparenting and updates occur in UISystem.BeginRender.
 */
extern "C" {
#include "mono.h"
#include "system_overlay.h"
#ifndef DEXCORE_OVERLAY_UI_TEST
#include "../../common/memory.h"
#include "../../common/HDE/HDE64.h"
#include <orbis/libkernel.h>
#else
#include "overlay_ui_test_api.h"
#endif
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
extern uint32_t mono_gchandle_new(void *,int);
extern void *mono_gchandle_get_target(uint32_t);
extern void mono_gchandle_free(uint32_t);
extern void *mono_class_get_parent(const void *);
extern int mono_class_is_valuetype(const void *);
}

namespace {
const char *status_text="Waiting for native UI renderer";
void *ui_image;
const char *base_namespace;
const char *ui_namespace;
void *widget_class,*panel_class,*label_class,*scene_class,*font_class,*color_class;
void *append_method,*remove_method;
void *area_class,*area_instance_get,*top_scene_method,*game_scene_method,*root_get;
uint32_t game_name_handle;
int installed;
pthread_mutex_t install_lock=PTHREAD_MUTEX_INITIALIZER;
void (*original_begin_render)(void);
uint64_t next_update;
bool disabled=false;
bool widgets_ready=false;

struct Slot {uint32_t root,panel,label,revision;};
Slot slots[2];

void set_status(const char *text){__atomic_store_n(&status_text,text,__ATOMIC_RELEASE);}
bool invoke(void *method,void *instance,const void **args,void **result=nullptr)
{
    if(!method)return false;
    const void *exception=nullptr;
    void *value=mono_runtime_invoke(method,instance,args,&exception);
    if(exception){set_status("Native UI rejected an overlay operation");return false;}
    if(result)*result=value;
    return true;
}
void *method(void *klass,const char *name,int count)
{
    for(void *p=klass;p;p=mono_class_get_parent(p)) {
        void *m=mono_class_get_method_from_name(p,name,count);
        if(m)return m;
    }
    return nullptr;
}
void *property(void *klass,const char *name,bool setter)
{
    for(void *p=klass;p;p=mono_class_get_parent(p)) {
        void *prop=mono_class_get_property_from_name(p,name);
        if(prop)return setter?mono_property_get_set_method(prop):mono_property_get_get_method(prop);
    }
    return nullptr;
}
bool set(void *klass,void *instance,const char *name,const void *value,bool required=true)
{
    void *m=property(klass,name,true);
    if(!m)return !required;
    const void *args[]={value};
    return invoke(m,instance,args);
}
void release(uint32_t &handle)
{
    if(handle)mono_gchandle_free(handle);
    handle=0;
}
void *target(uint32_t handle){return handle?mono_gchandle_get_target(handle):nullptr;}
uint32_t new_object(void *klass,int count=0,const void **args=nullptr)
{
    if(!klass)return 0;
    void *object=mono_object_new(Root_Domain,klass);
    if(!object)return 0;
    uint32_t handle=mono_gchandle_new(object,1);
    if(!handle)return 0;
    void *instance=mono_class_is_valuetype(klass)?mono_object_unbox(target(handle)):target(handle);
    if(!invoke(method(klass,".ctor",count),instance,args)){release(handle);return 0;}
    return handle;
}
bool resolve_ui(void)
{
    if(!Root_Domain || !App_Exe)return false;
    if(!ui_image) {
        const char *sandbox=sceKernelGetFsSandboxRandomWord();
        if(!sandbox)return false;
        const char *files[]={"Sce.PlayStation.PUI.dll","Sce.PlayStation.HighLevel.UI2.dll"};
        for(unsigned i=0;i<2 && !ui_image;i++) {
            char path[260];snprintf(path,sizeof(path),"/%s/common/lib/%s",sandbox,files[i]);
            ui_image=mono_get_image(path);
            if(ui_image) {
                base_namespace=i?"Sce.PlayStation.HighLevel.UI2":"Sce.PlayStation.PUI";
                ui_namespace=i?"Sce.PlayStation.HighLevel.UI2":"Sce.PlayStation.PUI.UI2";
            }
        }
    }
    return ui_image!=nullptr;
}
bool bind_widgets(void)
{
    widget_class=mono_class_from_name(ui_image,ui_namespace,"Widget");
    panel_class=mono_class_from_name(ui_image,ui_namespace,"Panel");
    label_class=mono_class_from_name(ui_image,ui_namespace,"Label");
    scene_class=mono_class_from_name(ui_image,ui_namespace,"Scene");
    font_class=mono_class_from_name(ui_image,ui_namespace,"UIFont");
    color_class=mono_class_from_name(ui_image,base_namespace,"UIColor");
    area_class=mono_class_from_name(App_Exe,"Sce.Vsh.ShellUI.TopMenu","AreaManager");
    void *layer=mono_class_from_name(App_Exe,"Sce.Vsh.ShellUI.AppSystem","LayerManager");
    area_instance_get=property(area_class,"Instance",false);
    top_scene_method=method(area_class,"GetTopScene",0);
    game_scene_method=method(layer,"FindContainerSceneByPath",1);
    root_get=property(scene_class,"RootWidget",false);
    append_method=method(widget_class,"AppendChild",1);
    remove_method=method(widget_class,"RemoveFromParent",0);
    return widget_class && panel_class && label_class && font_class && color_class &&
           root_get && append_method && remove_method &&
           property(label_class,"Text",true) && (top_scene_method || game_scene_method);
}
bool clear_slot(Slot &slot)
{
    if(target(slot.panel)) {
        bool no=false;
        set(widget_class,target(slot.panel),"Visible",&no,false);
        if(!invoke(remove_method,target(slot.panel),nullptr))return false;
    }
    release(slot.label);release(slot.panel);release(slot.root);slot.revision=0;
    return true;
}
bool create_panel(Slot &slot)
{
    slot.panel=new_object(panel_class);slot.label=new_object(label_class);
    if(!slot.panel || !slot.label)return false;
    void *panel=target(slot.panel),*label=target(slot.label);
    float x=24,y=76,width=560,height=208,lx=12,ly=10,lw=536,lh=188;
    bool yes=true;
    if(!set(widget_class,panel,"Name",Mono_New_String("DEXCORE.SystemInformation")) ||
       !set(widget_class,panel,"X",&x) || !set(widget_class,panel,"Y",&y) ||
       !set(widget_class,panel,"Width",&width) || !set(widget_class,panel,"Height",&height) ||
       !set(widget_class,label,"X",&lx) || !set(widget_class,label,"Y",&ly) ||
       !set(widget_class,label,"Width",&lw) || !set(widget_class,label,"Height",&lh))return false;
    int size=20,style=0,weight=3;
    const void *font_args[]={&size,&style,&weight};
    uint32_t font=new_object(font_class,3,font_args);
    if(!font)return false;
    void *font_value=mono_class_is_valuetype(font_class)?mono_object_unbox(target(font)):target(font);
    bool ok=set(label_class,label,"Font",font_value);release(font);
    float red=0.025f,green=0.025f,blue=0.04f,alpha=0.78f;
    const void *color_args[]={&red,&green,&blue,&alpha};
    uint32_t color=new_object(color_class,4,color_args);
    if(!color)return false;
    void *color_value=mono_class_is_valuetype(color_class)?mono_object_unbox(target(color)):target(color);
    ok=set(panel_class,panel,"BackgroundColor",color_value) && ok;release(color);
    red=green=blue=alpha=1.0f;
    color=new_object(color_class,4,color_args);
    if(!color)return false;
    color_value=mono_class_is_valuetype(color_class)?mono_object_unbox(target(color)):target(color);
    ok=set(label_class,label,"TextColor",color_value) && ok;release(color);
    int align=0;
    ok=set(label_class,label,"VerticalAlignment",&align,false) && ok;
    ok=set(label_class,label,"HorizontalAlignment",&align,false) && ok;
    ok=set(label_class,label,"FitHeightToText",&yes,false) && ok;
    const void *args[]={label};
    return ok && invoke(append_method,panel,args);
}
void *scene_root(void *scene)
{
    void *root=nullptr;
    if(scene)invoke(root_get,scene,nullptr,&root);
    return root;
}
bool update_slot(Slot &slot,void *root,const DexcoreOverlayFrame &frame)
{
    if(!root)return !slot.root || clear_slot(slot);
    if(target(slot.root)!=root) {
        /* Pin the new parent before any managed allocations or detach calls. */
        uint32_t parent=mono_gchandle_new(root,1);
        if(!parent)return false;
        if(!clear_slot(slot)){release(parent);return false;}
        slot.root=parent;
        if(!create_panel(slot)){clear_slot(slot);return false;}
        const void *args[]={target(slot.panel)};
        if(!invoke(append_method,target(slot.root),args)){clear_slot(slot);return false;}
        slot.revision=~frame.revision;
    }
    if(slot.revision!=frame.revision) {
        if(!set(label_class,target(slot.label),"Text",Mono_New_String(frame.text))) {
            clear_slot(slot);return false;
        }
        slot.revision=frame.revision;
    }
    bool yes=true;
    set(widget_class,target(slot.panel),"Visible",&yes,false);
    return true;
}
void render_tick(void)
{
    if(!dexcore_overlay_enabled()) {
        if(!disabled) {
            bool first=clear_slot(slots[0]);
            bool second=clear_slot(slots[1]);
            disabled=first && second;next_update=0;
        }
        return;
    }
    disabled=false;
    uint64_t now=sceKernelGetProcessTime();
    if(next_update && now<next_update)return;
    next_update=now+250000;
    DexcoreOverlayFrame frame;
    if(!dexcore_overlay_get_frame(&frame) || !frame.enabled)return;
    if(!widgets_ready) {
        widgets_ready=bind_widgets();
        if(!widgets_ready){set_status("Required native overlay widgets unavailable");return;}
    }
    if(!game_name_handle)game_name_handle=mono_gchandle_new(Mono_New_String("Game"),1);
    void *game_scene=nullptr,*area=nullptr,*top_scene=nullptr;
    const void *args[]={target(game_name_handle)};
    if(game_name_handle)invoke(game_scene_method,nullptr,args,&game_scene);
    /* Pin Game before resolving the other scene, which can allocate. */
    void *root=scene_root(game_scene);
    uint32_t game_root=root?mono_gchandle_new(root,1):0;
    invoke(area_instance_get,nullptr,nullptr,&area);
    if(area)invoke(top_scene_method,area,nullptr,&top_scene);
    root=scene_root(top_scene);
    uint32_t top_root=root?mono_gchandle_new(root,1):0;
    bool ok=update_slot(slots[0],target(game_root),frame);
    ok=update_slot(slots[1],target(top_root)==target(game_root)?nullptr:target(top_root),frame) && ok;
    release(game_root);release(top_root);
    if(!ok)set_status("Native overlay update failed; will retry");
    else if(slots[0].panel || slots[1].panel)set_status("Live overlay: updates every second");
    else set_status("Waiting for a native overlay scene");
}
void begin_render(void)
{
    render_tick();
    original_begin_render();
}
#ifndef DEXCORE_OVERLAY_UI_TEST
bool relocatable_prologue(uintptr_t address)
{
    unsigned offset=0;
    while(offset<14) {
        hde64s instruction;
        unsigned length=hde64_disasm((const void *)(address+offset),&instruction);
        if(!length || instruction.flags&(F_ERROR|F_RELATIVE))return false;
        /* Existing HEN trampolines do not relocate RIP-relative operands. */
        if((instruction.flags&F_MODRM) && instruction.modrm_mod==0 && instruction.modrm_rm==5)return false;
        if(instruction.opcode==0xC3 || instruction.opcode==0xC2)return false;
        offset+=length;
    }
    return true;
}
#endif
}

extern "C" const char *dexcore_overlay_ui_status(void)
{
    return __atomic_load_n(&status_text,__ATOMIC_ACQUIRE);
}
extern "C" void dexcore_overlay_ui_install(void)
{
#ifndef DEXCORE_OVERLAY_UI_TEST
    pthread_mutex_lock(&install_lock);
    if(installed){pthread_mutex_unlock(&install_lock);return;}
    if(!resolve_ui()){set_status("Native UI library not ready");pthread_mutex_unlock(&install_lock);return;}
    void *ui=mono_class_from_name(ui_image,base_namespace,"UISystem");
    void *m=method(ui,"BeginRender",0);
    uintptr_t address=m?(uintptr_t)mono_aot_get_method(Root_Domain,m):0;
    if(!address || !relocatable_prologue(address)) {
        set_status("Unsupported native render hook; overlay not installed");
        printf("DEXCORE overlay: native BeginRender unavailable or unsafe to relocate\n");
        pthread_mutex_unlock(&install_lock);return;
    }
    uintptr_t trampoline=CreatePrologueHook(address,14);
    if(!trampoline){set_status("Native overlay trampoline unavailable");pthread_mutex_unlock(&install_lock);return;}
    original_begin_render=(void (*)(void))trampoline;
    uint8_t jump[14]={0xff,0x25,0,0,0,0};
    uintptr_t destination=(uintptr_t)begin_render;
    memcpy(jump+6,&destination,sizeof(destination));
    if(sys_proc_rw(getpid(),address,jump,sizeof(jump),1)==0 && !memcmp((void *)address,jump,sizeof(jump))) {
        installed=1;set_status("Native overlay renderer ready");
    } else set_status("Unable to install native overlay render hook");
    pthread_mutex_unlock(&install_lock);
#endif
}
