// This file is generated automatically. Do not edit!
// It contains the registration code for all built-in fonts compiled into PiGFX.

// Externs from binary_assets.s
extern unsigned char G_SYSTEM_8X16_GLYPHS[];
extern unsigned char G_SYSTEM_8X24_GLYPHS[];
extern unsigned char G_VT100_10X20_GLYPHS[];
extern unsigned char G_VT220_10X20_GLYPHS[];
extern unsigned char G_VT220_12X24_GLYPHS[];
extern unsigned char G_VT220_14X28_GLYPHS[];
extern unsigned char G_VT220_16X32_GLYPHS[];



void gfx_register_builtin_fonts(void)
{
    // Register all built-in fonts
    // 8x16 System Font is the system default font (index 0)

    font_registry_register("System 8x16", 8, 16, G_SYSTEM_8X16_GLYPHS, font_get_glyph_address);
    font_registry_register("System 8x24", 8, 24, G_SYSTEM_8X24_GLYPHS, font_get_glyph_address);
    font_registry_register("Vt100 10x20", 10, 20, G_VT100_10X20_GLYPHS, font_get_glyph_address);
    font_registry_register("Vt220 10x20", 10, 20, G_VT220_10X20_GLYPHS, font_get_glyph_address);
    font_registry_register("Vt220 12x24", 12, 24, G_VT220_12X24_GLYPHS, font_get_glyph_address);
    font_registry_register("Vt220 14x28", 14, 28, G_VT220_14X28_GLYPHS, font_get_glyph_address);
    font_registry_register("Vt220 16x32", 16, 32, G_VT220_16X32_GLYPHS, font_get_glyph_address);
}
