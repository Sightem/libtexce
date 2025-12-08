#include "tex_renderer.h"
#include <stdlib.h>
#include <string.h>

TeX_Renderer* tex_renderer_create(void) { return tex_renderer_create_sized(TEX_RENDERER_DEFAULT_SLAB_SIZE); }

TeX_Renderer* tex_renderer_create_sized(size_t slab_size)
{
	TeX_Renderer* r = (TeX_Renderer*)calloc(1, sizeof(TeX_Renderer));
	if (!r)
		return NULL;

	arena_init_slab(&r->arena, slab_size);
	if (r->arena.failed)
	{
		free(r);
		return NULL;
	}

	r->visible_lines = NULL;
	r->window_y_start = 0;
	r->window_y_end = 0;
	r->cached_layout = NULL;

	return r;
}

void tex_renderer_destroy(TeX_Renderer* r)
{
	if (!r)
		return;

	arena_free_all(&r->arena);
	free(r);
}

void tex_renderer_invalidate(TeX_Renderer* r)
{
	if (!r)
		return;

	arena_reset(&r->arena);
	r->visible_lines = NULL;
	r->window_y_start = 0;
	r->window_y_end = 0;
	r->cached_layout = NULL;
}
