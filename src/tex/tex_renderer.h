#ifndef TEX_TEX_RENDERER_H
#define TEX_TEX_RENDERER_H

#include "tex_arena.h"
#include "tex_internal.h"

#define TEX_RENDERER_DEFAULT_SLAB_SIZE ((size_t)40 * 1024)

#define TEX_RENDERER_PADDING 240

struct TeX_Layout;

typedef struct TeX_Renderer
{
	TexArena arena; // the slab arena for transient allocations
	TeX_Line* visible_lines; // head of current window's line list
	TeX_Line** line_index; // optional index for large windows (binary search)
	int line_count; // number of lines in current window
	int window_y_start; // top of currently loaded window
	int window_y_end; // bottom of currently loaded window
	struct TeX_Layout* cached_layout; // layout currently hydrated (for hit check)
} TeX_Renderer;

// create a renderer with default slab size (40KB)
TeX_Renderer* tex_renderer_create(void);

// create a renderer with custom slab size
TeX_Renderer* tex_renderer_create_sized(size_t slab_size);

// destroy renderer and free slab
void tex_renderer_destroy(TeX_Renderer* r);

// invalidate cached window (forces rehydration on next draw)
void tex_renderer_invalidate(TeX_Renderer* r);

#endif // TEX_TEX_RENDERER_H
