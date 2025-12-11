
#ifndef TEX_TEX_H
#define TEX_TEX_H

#include <stddef.h>
#include "tex_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TeX_Layout TeX_Layout;
typedef struct TeX_Renderer TeX_Renderer;


// Parse input, calculate layout, return handle
// Input string is modified during parsing
// Returns NULL only on catastrophic failure; check tex_get_last_error() for errors
TeX_Layout* tex_format(char* input, int width, TeX_Config* config);

// Total rendered height in pixels (for scrollbar sizing)
int tex_get_total_height(TeX_Layout* layout);

// Create a renderer with default slab size (40KB)
TeX_Renderer* tex_renderer_create(void);

// Create a renderer with custom slab size
TeX_Renderer* tex_renderer_create_sized(size_t slab_size);

// Destroy renderer and free slab
void tex_renderer_destroy(TeX_Renderer* r);

// Draw document slice to current draw buffer
// Uses windowed rendering: only parses visible portion + padding
void tex_draw(TeX_Renderer* r, TeX_Layout* layout, int x, int y, int scroll_y);

// Free all resources
void tex_free(TeX_Layout* layout);

// Get renderer pool statistics (pass NULL for any stat you dont need)
void tex_renderer_get_stats(TeX_Renderer* r, size_t* peak_used, size_t* capacity, size_t* alloc_count,
                            size_t* reset_count);

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

#ifdef __cplusplus
}
#endif

#endif // TEX_TEX_H
