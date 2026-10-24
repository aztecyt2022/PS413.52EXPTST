/* Relocate whole instructions for a five-byte branch to a nearby relay.
 * HDE is the same decoder used by the upstream native HEN plugin.
 * RIP operands and relative branches are adjusted instead of copied blindly.
 * GPL-3.0-or-later.
 */
#include "overlay_hook.h"
#include <string.h>
#ifdef DEXCORE_HOOK_TEST
#include "vendor/hde64/HDE64.h"
#else
#include "../../common/HDE/HDE64.h"
#endif

int dexcore_overlay_hook_plan(const uint8_t *source,uintptr_t source_address,
    uint8_t *out,uintptr_t out_address,size_t capacity,size_t *stolen)
{
    hde64s decoded[16];size_t offsets[16],count=0,total=0;
    if(!source || !out || !stolen || sizeof(uintptr_t)!=8)return 0;
    while(total<5) {
        if(count>=16)return 0;
        unsigned length=hde64_disasm(source+total,&decoded[count]);
        hde64s *h=&decoded[count];
        if(!length || h->flags&F_ERROR || h->p_67 || h->opcode==0xc2 || h->opcode==0xc3)return 0;
        offsets[count++]=total;total+=length;
    }
    if(total+14>capacity)return 0;
    memcpy(out,source,total);
    for(size_t i=0;i<count;i++) {
        hde64s *h=&decoded[i];size_t at=offsets[i];
        if(h->flags&F_RELATIVE) {
            size_t width=(h->flags&F_IMM32)?4:(h->flags&F_IMM8)?1:0;
            if(!width || h->len<width)return 0;
            int64_t old=width==4?(int32_t)h->imm.imm32:(int8_t)h->imm.imm8;
            uintptr_t destination=source_address+at+h->len+old;
            if(destination>=source_address && destination<source_address+total)
                destination=out_address+(destination-source_address);
            int64_t delta=(int64_t)destination-(int64_t)(out_address+at+h->len);
            if(width==4) {
                if(delta<INT32_MIN || delta>INT32_MAX)return 0;
                int32_t value=(int32_t)delta;memcpy(out+at+h->len-4,&value,4);
            } else {
                if(delta<INT8_MIN || delta>INT8_MAX)return 0;
                out[at+h->len-1]=(uint8_t)(int8_t)delta;
            }
        }
        if((h->flags&F_MODRM) && h->modrm_mod==0 && h->modrm_rm==5) {
            size_t imm=(h->flags&F_IMM8?1:0)+(h->flags&F_IMM16?2:0)+
                       (h->flags&F_IMM32?4:0)+(h->flags&F_IMM64?8:0);
            if(h->len<4+imm)return 0;
            int64_t destination=(int64_t)(source_address+at+h->len)+(int32_t)h->disp.disp32;
            int64_t delta=destination-(int64_t)(out_address+at+h->len);
            if(delta<INT32_MIN || delta>INT32_MAX)return 0;
            int32_t value=(int32_t)delta;memcpy(out+at+h->len-imm-4,&value,4);
        }
    }
    const uint8_t jump[]={0xff,0x25,0,0,0,0};
    memcpy(out+total,jump,sizeof(jump));
    uintptr_t resume=source_address+total;memcpy(out+total+6,&resume,8);
    *stolen=total;return 1;
}
