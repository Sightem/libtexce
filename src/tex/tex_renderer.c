#include "tex_renderer.h"
#include <stdlib.h>
#include <string.h>
#include "tex.h"

TeX_Renderer* tex_renderer_create(void) { return tex_renderer_create_sized(TEX_RENDERER_DEFAULT_SLAB_SIZE); }

TeX_Renderer* tex_renderer_create_sized(size_t slab_size)
{
	TeX_Renderer* r = (TeX_Renderer*)calloc(1, sizeof(TeX_Renderer));
	if (!r)
		return NULL;

	if (pool_init(&r->pool, slab_size) != 0)
	{
		free(r);
		return NULL;
	}

	r->line_count = 0;
	r->window_y_start = 0;
	r->window_y_end = 0;
	r->cached_layout = NULL;

	return r;
}

void tex_renderer_destroy(TeX_Renderer* r)
{
	if (!r)
		return;

	pool_free(&r->pool);
	free(r);
}

void tex_renderer_invalidate(TeX_Renderer* r)
{
	if (!r)
		return;

	pool_reset(&r->pool);
	r->line_count = 0;
	r->window_y_start = 0;
	r->window_y_end = 0;
	r->cached_layout = NULL;
}

void tex_renderer_get_stats(TeX_Renderer* r, size_t* peak_used, size_t* capacity, size_t* alloc_count,
                            size_t* reset_count)
{
	if (!r)
	{
		if (peak_used)
			*peak_used = 0;
		if (capacity)
			*capacity = 0;
		if (alloc_count)
			*alloc_count = 0;
		if (reset_count)
			*reset_count = 0;
		return;
	}

	if (peak_used)
		*peak_used = r->pool.peak_used;
	if (capacity)
		*capacity = r->pool.capacity;
	if (alloc_count)
		*alloc_count = r->pool.alloc_count;
	if (reset_count)
		*reset_count = r->pool.reset_count;
}
