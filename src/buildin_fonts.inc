// This file is generated automatically. Do not edit!
// It contains the registration code for all built-in fonts compiled into PiGFX.

// Externs from binary_assets.s
extern unsigned char G_SYSTEM_8X16_GLYPHS[];
extern unsigned char G_SYSTEM_8X24_GLYPHS[];
extern unsigned char G_VT100_10X20_GLYPHS[];


void gfx_register_builtin_fonts(void)
{
    // Register all built-in fonts
    // 8x16 System Font is the system default font (index 0)

    font_registry_register("System 8x16", 8, 16, G_SYSTEM_8X16_GLYPHS, font_get_glyph_address);
    font_registry_register("System 8x24", 8, 24, G_SYSTEM_8X24_GLYPHS, font_get_glyph_address);
    font_registry_register("Vt100 10x20", 10, 20, G_VT100_10X20_GLYPHS, font_get_glyph_address);
}
