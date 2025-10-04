.section .rodata

.global G_SYSTEM_8X16_GLYPHS
.align 4
G_SYSTEM_8X16_GLYPHS: .incbin "fonts/bin/System-8x16.bin"

.global G_SYSTEM_8X24_GLYPHS
.align 4
G_SYSTEM_8X24_GLYPHS: .incbin "fonts/bin/System-8x24.bin"

.global G_VT100_10X20_GLYPHS
.align 4
G_VT100_10X20_GLYPHS: .incbin "fonts/bin/VT100-10x20.bin"
