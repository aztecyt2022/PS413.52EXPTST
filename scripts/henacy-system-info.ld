/* Addresses belong to the checked plugin_mono image, never Sony firmware. */
ENTRY(HNCY_SystemInfoBoot)
Root_Domain = 0x202a0;
App_Exe = 0x202a8;
MainThreadCheck = 0x1d30;
mono_get_image = 0x59f0;
mono_class_from_name = 0xabe0;
mono_class_get_property_from_name = 0xabf0;
mono_property_get_get_method = 0xac00;
mono_property_get_set_method = 0xacd0;
mono_class_get_method_from_name = 0xac20;
mono_aot_get_method = 0xac30;
mono_runtime_invoke = 0xac10;
mono_object_new = 0xac90;
mono_string_new = 0xac40;
mono_string_to_utf8 = 0xacb0;
mono_free = 0xacc0;
sceKernelGetFsSandboxRandomWord = 0xaaa0;
getpid = 0xaab0;
sys_proc_rw = 0x3600;
snprintf = 0xa9c0;
printf = 0xa9d0;
hde64_disasm = 0x2320;
hncy_text_trampoline = 0x13f80;
SECTIONS {
  .text 0x12040 : { *(.text .text.*) }
  .rodata 0xf620 : { *(.rodata .rodata.*) }
  .bss 0x22800 (NOLOAD) : { *(.bss .bss.* COMMON) }
  /DISCARD/ : { *(.eh_frame*) *(.comment) *(.note*) }
}
ASSERT(ADDR(.text) + SIZEOF(.text) <= 0x13f80, "System info code exceeds unused RX padding")
ASSERT(ADDR(.rodata) + SIZEOF(.rodata) <= 0x1081a, "System info constants exceed the retired PNG slot")
ASSERT(SIZEOF(.bss) <= 0x400, "System info state exceeds reserved BSS")
