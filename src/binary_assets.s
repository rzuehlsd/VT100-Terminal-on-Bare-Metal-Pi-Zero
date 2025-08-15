.section .rodata

.global G_FONT8X8_GLYPHS
.align 4
G_FONT8X8_GLYPHS: .incbin "fonts/font8x8.bin"

.global G_FONT8X24_GLYPHS
.align 4
G_FONT8X24_GLYPHS: .incbin "fonts/TRSfont8x24.bin"

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
