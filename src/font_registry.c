//
// font_registry.c  
// Generic Font Management System Implementation
//

#include "font_registry.h"
#include "gfx.h"
#include "myString.h"
#include <string.h>



// Global font registry
static font_registry_t g_font_registry;



char* truncate_name(const char* name)
{
    static char truncated[32];
    if(strlen(name) < sizeof(truncated))
    {
        return (char*)name; // No truncation needed
    }   
    strncpy(truncated, name, sizeof(truncated) - 1);
    truncated[sizeof(truncated) - 1] = '\0'; // Ensure null-termination
    return truncated;
}


void font_registry_init(void)
{
    g_font_registry.count = 0;
    g_font_registry.current_index = 0;
    
    // Clear all font entries
    for (int i = 0; i < MAX_FONTS; i++)
    {
        strcpy(g_font_registry.fonts[i].name, "");
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

    // max size for name 32
    strcpy(font->name, truncate_name(name));
    font->width = width;
    font->height = height;
    font->data = data;
    font->get_glyph = get_glyph;
    font->is_valid = 0; // Will be validated later
    
    g_font_registry.count++;
    
    return index; // Return the index of the newly registered font
}

int font_registry_set_by_index(int index)
{
    if (index < 0 || index >= g_font_registry.count)
    {
        return -1; // Invalid index
    }
    
    
    // Set the font in the graphics system
    g_font_registry.current_index = index;
    
    return index; // font type is set
}

int font_registry_set_by_name(const char* name)
{
    for (int i = 0; i < g_font_registry.count; i++)
    {
        if (strlen(g_font_registry.fonts[i].name) != 0)
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


