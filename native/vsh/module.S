/* OpenOrbis-compatible SPRX module parameter: size, magic, SDK version. */
.section .data.sce_module_param,"aw",@progbits
.balign 8
.global _sceModuleParam
_sceModuleParam:
.quad 0x18
.quad 0x13c13f4bf
.quad 0x1000051
.section .note.GNU-stack,"",@progbits
