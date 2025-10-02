//
// font_registry.h
// Generic Font Management System for PiGFX
//
// This system provides a unified interface for managing fonts in PiGFX binary format.
// All fonts are registered in a central registry with metadata and validation.
//

#ifndef FONT_REGISTRY_H_
#define FONT_REGISTRY_H_

#define MAX_FONTS 16    // Maximum number of fonts that can be registered

// Font descriptor structure containing all metadata for a font
typedef struct {
    char name[32];                                 // Human-readable name
    int width;                                     // Character width in pixels
    int height;                                    // Character height in pixels
    const unsigned char* data;                     // Pointer to binary font data
    unsigned char* (*get_glyph)(unsigned int c);   // Glyph address function
    int is_valid;                                  // Validation flag
} font_descriptor_t;

// Font registry structure
typedef struct {
    font_descriptor_t fonts[MAX_FONTS];
    int count;                                     // Number of registered fonts
    int current_index;                             // Currently active font index
} font_registry_t;

// Font registry API functions

/**
 * Initialize the font registry system
 * Must be called before any other font registry functions
 */
void font_registry_init(void);

/**
 * Register a new font in the registry
 * @param name Human-readable font name
 * @param width Character width in pixels
 * @param height Character height in pixels
 * @param data Pointer to binary font data
 * @param get_glyph Function to get glyph address for a character
 * @return Font index if successful, -1 if failed
 */
int font_registry_register(const char* name, int width, int height, 
                          const unsigned char* data,
                          unsigned char* (*get_glyph)(unsigned int c));

/**
 * Set the current font by registry index
 * @param index Font index in the registry
 * @return 1 if successful, 0 if failed
 */
int font_registry_set_by_index(int index);

/**
 * Set the current font by name
 * @param name Font name to search for
 * @return 1 if successful, 0 if failed
 */
int font_registry_set_by_name(const char* name);

/**
 * Get the number of registered fonts
 * @return Number of fonts in the registry
 */
int font_registry_get_count(void);

/**
 * Get font information by index
 * @param index Font index in the registry
 * @return Pointer to font descriptor, or NULL if invalid index
 */
const font_descriptor_t* font_registry_get_info(int index);

/**
 * Get the current font index
 * @return Current font index, or -1 if no font is set
 */
int font_registry_get_current_index(void);


/**
 * Find a font by dimensions (width x height)
 * @param width Character width in pixels
 * @param height Character height in pixels
 * @return Font index if found, -1 if not found
 */
int font_registry_find_by_dimensions(int width, int height);



#endif /* FONT_REGISTRY_H_ */
