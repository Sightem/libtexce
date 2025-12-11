#ifndef TEX_TEX_RENDERER_H
#define TEX_TEX_RENDERER_H

#include "tex_internal.h"
#include "tex_pool.h"

#define TEX_RENDERER_DEFAULT_SLAB_SIZE ((size_t)40 * 1024)
#define TEX_RENDERER_MAX_LINES 64
#define TEX_RENDERER_PADDING 240

struct TeX_Layout;

typedef struct TeX_Renderer
{
	UnifiedPool pool; // the slab for transient allocations
	TeX_Line lines[TEX_RENDERER_MAX_LINES]; // fixed array for visible lines
	int line_count; // number of lines in current window
	int window_y_start; // top of currently loaded window
	int window_y_end; // bottom of currently loaded window
	struct TeX_Layout* cached_layout; // layout currently hydrated (for hit check)
} TeX_Renderer;

// invalidate cached window (forces rehydration on next draw)
void tex_renderer_invalidate(TeX_Renderer* r);

#endif // TEX_TEX_RENDERER_H
