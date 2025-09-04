.section .rodata

.global G_FONT8X16_GLYPHS
.align 4
G_FONT8X16_GLYPHS: .incbin "fonts/TRSfont8x16.bin"

@ Spleen fonts
.global G_SPLEEN6X12_GLYPHS
.align 4
G_SPLEEN6X12_GLYPHS: .incbin "fonts/spleen-6x12.bin"

.global G_SPLEEN8X16_GLYPHS
.align 4
G_SPLEEN8X16_GLYPHS: .incbin "fonts/spleen-8x16.bin"

.global G_SPLEEN12X24_GLYPHS
.align 4
G_SPLEEN12X24_GLYPHS: .incbin "fonts/spleen-12x24.bin"

.global G_SPLEEN16X32_GLYPHS
.align 4
G_SPLEEN16X32_GLYPHS: .incbin "fonts/spleen-16x32.bin"

.global G_SPLEEN32X64_GLYPHS
.align 4
G_SPLEEN32X64_GLYPHS: .incbin "fonts/spleen-32x64.bin"

.global G_STARTUP_LOGO
.align 4
G_STARTUP_LOGO: .incbin "sprite/rc2014logo.bin"

@ VT220 fonts
.global G_VT220_6X12_GLYPHS
.align 4
G_VT220_6X12_GLYPHS: .incbin "fonts/VT220-6x12.bin"

.global G_VT220_8X16_GLYPHS
.align 4
G_VT220_8X16_GLYPHS: .incbin "fonts/VT220-8x16.bin"

.global G_VT220_12X24_GLYPHS
.align 4
G_VT220_12X24_GLYPHS: .incbin "fonts/VT220-12x24.bin"

.global G_VT220_16X32_GLYPHS
.align 4
G_VT220_16X32_GLYPHS: .incbin "fonts/VT220-16x32.bin"

.global G_VT220_32X64_GLYPHS
.align 4
G_VT220_32X64_GLYPHS: .incbin "fonts/VT220-32x64.bin"


