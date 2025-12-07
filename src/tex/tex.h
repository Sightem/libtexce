
#ifndef TEX_TEX_H
#define TEX_TEX_H

#include "tex_types.h"

typedef struct TeX_Layout TeX_Layout;


// Parse input, calculate layout, return handle
// Input string is modified during parsing
// Returns NULL only on catastrophic failure; check tex_get_last_error() for errors
TeX_Layout* tex_format(char* input, int width, TeX_Config* config);

// Total rendered height in pixels (for scrollbar sizing)
int tex_get_total_height(TeX_Layout* layout);

// Draw document slice to current draw buffer
void tex_draw(TeX_Layout* layout, int x, int y, int scroll_y);

// Free all resources
void tex_free(TeX_Layout* layout);

// Get error code from last operation
TeX_Error tex_get_last_error(TeX_Layout* layout);

// Get human-readable error message (static string, never NULL)
const char* tex_get_error_message(TeX_Layout* layout);

// Get error detail value (offset, depth, count, etc.)
int tex_get_error_value(TeX_Layout* layout);


#if defined(TEX_DIRECT_RENDER)
#if defined(TEX_USE_FONTLIB)
struct fontlib_font_t;
typedef struct fontlib_font_t fontlib_font_t;
void tex_draw_set_fonts(fontlib_font_t* main, fontlib_font_t* script);
#else
void tex_draw_set_fonts(void* main, void* script);
#endif
#endif


#endif // TEX_TEX_H
