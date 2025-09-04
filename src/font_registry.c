//
// font_registry.c  
// Generic Font Management System Implementation
//

#include "font_registry.h"
#include "gfx.h"

// External font data symbols and glyph functions
extern unsigned char G_FONT8X16_GLYPHS;
extern unsigned char G_SPLEEN6X12_GLYPHS;
extern unsigned char G_SPLEEN8X16_GLYPHS;
extern unsigned char G_SPLEEN12X24_GLYPHS;
extern unsigned char G_SPLEEN16X32_GLYPHS;
extern unsigned char G_SPLEEN32X64_GLYPHS;

// Externs for VT220 font data (only those defined in binary_assets.s)
extern unsigned char G_VT220_6X12_GLYPHS; // <-- Add this line
extern unsigned char G_VT220_8X16_GLYPHS;
extern unsigned char G_VT220_12X24_GLYPHS;
extern unsigned char G_VT220_16X32_GLYPHS;
extern unsigned char G_VT220_32X64_GLYPHS;

// External glyph address functions
extern unsigned char* font_get_glyph_address_8x16(unsigned int c);
extern unsigned char* font_get_glyph_address_spleen6x12(unsigned int c);
extern unsigned char* font_get_glyph_address_spleen8x16(unsigned int c);
extern unsigned char* font_get_glyph_address_spleen12x24(unsigned int c);
extern unsigned char* font_get_glyph_address_spleen16x32(unsigned int c);
extern unsigned char* font_get_glyph_address_spleen32x64(unsigned int c);

// Glyph address functions for VT220 fonts (matching only defined assets)
unsigned char* font_get_glyph_address_vt220_6x12(unsigned int c) {
    if (c >= 256) return 0;
    return ((unsigned char*)&G_VT220_6X12_GLYPHS) + c * 6*12;
}
unsigned char* font_get_glyph_address_vt220_8x16(unsigned int c) {
    if (c >= 256) return 0;
    return ((unsigned char*)&G_VT220_8X16_GLYPHS) + c * 8*16;
}
unsigned char* font_get_glyph_address_vt220_12x24(unsigned int c) {
    if (c >= 256) return 0;
    return ((unsigned char*)&G_VT220_12X24_GLYPHS) + c * 12*24;
}
unsigned char* font_get_glyph_address_vt220_16x32(unsigned int c) {
    if (c >= 256) return 0;
    return ((unsigned char*)&G_VT220_16X32_GLYPHS) + c * 16*32;
}
unsigned char* font_get_glyph_address_vt220_32x64(unsigned int c) {
    if (c >= 256) return 0;
    return ((unsigned char*)&G_VT220_32X64_GLYPHS) + c * 32*64;
}

// Global font registry
static font_registry_t g_font_registry;

void font_registry_init(void)
{
    g_font_registry.count = 0;
    g_font_registry.current_index = -1;
    
    // Clear all font entries
    for (int i = 0; i < MAX_FONTS; i++)
    {
        g_font_registry.fonts[i].name = 0;
        g_font_registry.fonts[i].width = 0;
        g_font_registry.fonts[i].height = 0;
        g_font_registry.fonts[i].data = 0;
        g_font_registry.fonts[i].get_glyph = 0;
        g_font_registry.fonts[i].is_valid = 0;
    }
}

int font_registry_register(const char* name, int width, int height,
                          const unsigned char* data,
                          unsigned char* (*get_glyph)(unsigned int c))
{
    if (g_font_registry.count >= MAX_FONTS)
    {
        return -1; // Registry full
    }
    
    int index = g_font_registry.count;
    font_descriptor_t* font = &g_font_registry.fonts[index];
    
    font->name = name;
    font->width = width;
    font->height = height;
    font->data = data;
    font->get_glyph = get_glyph;
    font->is_valid = 0; // Will be validated later
    
    g_font_registry.count++;
    
    // Validate the font
    if (font_registry_validate(index))
    {
        font->is_valid = 1;
        return index;
    }
    else
    {
        // Remove invalid font
        g_font_registry.count--;
        return -1;
    }
}

int font_registry_set_by_index(int index)
{
    if (index < 0 || index >= g_font_registry.count)
    {
        return 0; // Invalid index
    }
    
    if (!g_font_registry.fonts[index].is_valid)
    {
        return 0; // Invalid font
    }
    
    // Set the font in the graphics system
    g_font_registry.current_index = index;
    
    // Map font registry index to correct graphics system font type
    int font_type = -1;
    
    // Check font dimensions to determine the correct font type for gfx system
    font_descriptor_t* font = &g_font_registry.fonts[index];
    
    if (font->width == 8 && font->height == 16)
    {
        // Check if it's original, Spleen, or VT220 8x16
        if (font->get_glyph == font_get_glyph_address_8x16)
        {
            font_type = 1; // 8x16 original
        }
        else if (font->get_glyph == font_get_glyph_address_spleen8x16)
        {
            font_type = 7; // 8x16 Spleen
        }
        else if (font->get_glyph == font_get_glyph_address_vt220_8x16)
        {
            font_type = 8; // 8x16 VT220
        }
    }
    else if (font->width == 6 && font->height == 12)
    {
        if (font->get_glyph == font_get_glyph_address_spleen6x12)
        {
            font_type = 3; // 6x12 Spleen
        }
        else if (font->get_glyph == font_get_glyph_address_vt220_6x12)
        {
            font_type = 9; // 6x12 VT220
        }
    }
    else if (font->width == 12 && font->height == 24)
    {
        if (font->get_glyph == font_get_glyph_address_spleen12x24)
        {
            font_type = 4; // 12x24 Spleen
        }
        else if (font->get_glyph == font_get_glyph_address_vt220_12x24)
        {
            font_type = 10; // 12x24 VT220
        }
    }
    else if (font->width == 16 && font->height == 32)
    {
        if (font->get_glyph == font_get_glyph_address_spleen16x32)
        {
            font_type = 5; // 16x32 Spleen
        }
        else if (font->get_glyph == font_get_glyph_address_vt220_16x32)
        {
            font_type = 11; // 16x32 VT220
        }
    }
    else if (font->width == 32 && font->height == 64)
    {
        if (font->get_glyph == font_get_glyph_address_spleen32x64)
        {
            font_type = 6; // 32x64 Spleen
        }
        else if (font->get_glyph == font_get_glyph_address_vt220_32x64)
        {
            font_type = 12; // 32x64 VT220
        }
    }
    
    if (font_type >= 0)
    {
        gfx_term_set_font_by_type(font_type);
        return 1;
    }
    
    return 0; // Unknown font type
}

int font_registry_set_by_name(const char* name)
{
    for (int i = 0; i < g_font_registry.count; i++)
    {
        if (g_font_registry.fonts[i].name != 0)
        {
            // Simple string comparison (could be improved)
            const char* font_name = g_font_registry.fonts[i].name;
            const char* search_name = name;
            
            int match = 1;
            while (*font_name && *search_name)
            {
                if (*font_name != *search_name)
                {
                    match = 0;
                    break;
                }
                font_name++;
                search_name++;
            }
            
            if (match && *font_name == *search_name)
            {
                return font_registry_set_by_index(i);
            }
        }
    }
    
    return 0; // Font not found
}

int font_registry_get_count(void)
{
    return g_font_registry.count;
}

const font_descriptor_t* font_registry_get_info(int index)
{
    if (index < 0 || index >= g_font_registry.count)
    {
        return 0; // Invalid index
    }
    
    return &g_font_registry.fonts[index];
}

int font_registry_get_current_index(void)
{
    return g_font_registry.current_index;
}

int font_registry_validate(int index)
{
    if (index < 0 || index >= g_font_registry.count)
    {
        return 0; // Invalid index
    }
    
    font_descriptor_t* font = &g_font_registry.fonts[index];
    
    if (!font->get_glyph || !font->data)
    {
        return 0; // Missing essential components
    }
    
    // Test a few critical characters
    unsigned char* glyph;
    
    // Test space character (0x20)
    glyph = font->get_glyph(0x20);
    if (!glyph) return 0;
    
    // Test 'A' character (0x41)  
    glyph = font->get_glyph(0x41);
    if (!glyph) return 0;
    
    // Test '0' character (0x30)
    glyph = font->get_glyph(0x30);
    if (!glyph) return 0;
    
    return 1; // Font appears valid
}

int font_registry_find_by_dimensions(int width, int height)
{
    for (int i = 0; i < g_font_registry.count; i++)
    {
        if (g_font_registry.fonts[i].width == width && 
            g_font_registry.fonts[i].height == height &&
            g_font_registry.fonts[i].is_valid)
        {
            return i;
        }
    }
    
    return -1; // Not found
}

void font_registry_register_builtin_fonts(void)
{
    // Register all built-in fonts
    // 8x16 System Font is the system default font (index 0)
    
    font_registry_register("8x16 System Font", 8, 16, &G_FONT8X16_GLYPHS,
                          font_get_glyph_address_8x16);
    
    font_registry_register("Spleen 6x12", 6, 12, &G_SPLEEN6X12_GLYPHS,
                          font_get_glyph_address_spleen6x12);
    
    font_registry_register("Spleen 8x16", 8, 16, &G_SPLEEN8X16_GLYPHS,
                          font_get_glyph_address_spleen8x16);
    
    font_registry_register("Spleen 12x24", 12, 24, &G_SPLEEN12X24_GLYPHS,
                          font_get_glyph_address_spleen12x24);
    
    font_registry_register("Spleen 16x32", 16, 32, &G_SPLEEN16X32_GLYPHS,
                          font_get_glyph_address_spleen16x32);
    
    font_registry_register("Spleen 32x64", 32, 64, &G_SPLEEN32X64_GLYPHS,
                          font_get_glyph_address_spleen32x64);
    
    // Register VT220 fonts (only those defined in binary_assets.s)
    font_registry_register("VT220 6x12", 6, 12, &G_VT220_6X12_GLYPHS, font_get_glyph_address_vt220_6x12); // <-- Add this line
    font_registry_register("VT220 8x16", 8, 16, &G_VT220_8X16_GLYPHS, font_get_glyph_address_vt220_8x16);
    font_registry_register("VT220 12x24", 12, 24, &G_VT220_12X24_GLYPHS, font_get_glyph_address_vt220_12x24);
    font_registry_register("VT220 16x32", 16, 32, &G_VT220_16X32_GLYPHS, font_get_glyph_address_vt220_16x32);
    font_registry_register("VT220 32x64", 32, 64, &G_VT220_32X64_GLYPHS, font_get_glyph_address_vt220_32x64);
}
