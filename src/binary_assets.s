.section .rodata

.global G_SYSTEM_8X16_GLYPHS
.align 4
G_SYSTEM_8X16_GLYPHS: .incbin "fonts/bin/System-8x16.bin"

.global G_SYSTEM_8X24_GLYPHS
.align 4
G_SYSTEM_8X24_GLYPHS: .incbin "fonts/bin/System-8x24.bin"

.global G_VT220_10X20_GLYPHS
.align 4
G_VT220_10X20_GLYPHS: .incbin "fonts/bin/VT220-10x20.bin"

.global G_VT100_10X20_GLYPHS
.align 4
G_VT100_10X20_GLYPHS: .incbin "fonts/bin/VT100-10x20.bin"

.global G_VT220_12X24_GLYPHS
.align 4
G_VT220_12X24_GLYPHS: .incbin "fonts/bin/VT220-12x24.bin"

.global G_VT220_14X28_GLYPHS
.align 4
G_VT220_14X28_GLYPHS: .incbin "fonts/bin/VT220-14x28.bin"

.global G_VT220_16X32_GLYPHS
.align 4
G_VT220_16X32_GLYPHS: .incbin "fonts/bin/VT220-16x32.bin"
