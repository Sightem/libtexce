#include "tex_arena.h"
#include <stdlib.h>
#include <string.h>
#include "tex_util.h"

static TexArenaChunk* arena_new_chunk(TexArena* a)
{
	size_t chunk_size = TEX_ARENA_CHUNK_SIZE;
	if (a && a->current)
	{
		size_t prev = a->current->capacity ? a->current->capacity : TEX_ARENA_CHUNK_SIZE;
		size_t candidate = prev * 2;
		if (candidate != TEX_ARENA_CHUNK_SIZE)
			candidate = TEX_ARENA_CHUNK_SIZE;
		chunk_size = candidate;
	}
	TEX_TRACE("Attempting malloc of %u bytes... ",
	          (unsigned int)(sizeof(TexArenaChunk) - TEX_ARENA_CHUNK_SIZE + chunk_size));
	TexArenaChunk* c = (TexArenaChunk*)malloc(sizeof(TexArenaChunk) - TEX_ARENA_CHUNK_SIZE + chunk_size);
	if (!c)
	{
		TEX_TRACE("%s", "FAILED!");
		return NULL;
	}
	TEX_TRACE("Success. (Ptr: %p)", (void*)c);
	c->next = NULL;
	c->used = 0;
	c->capacity = chunk_size;
	return c;
}

void arena_init(TexArena* a)
{
	if (!a)
		return;
	memset(a, 0, sizeof(*a));
	TexArenaChunk* c = arena_new_chunk(a);
	if (!c)
	{
		a->failed = 1;
		return;
	}
	a->first = a->current = c;
	a->total_allocated = sizeof(TexArenaChunk) - TEX_ARENA_CHUNK_SIZE + c->capacity;
}

void arena_init_slab(TexArena* a, size_t slab_size)
{
	if (!a)
		return;
	memset(a, 0, sizeof(*a));

	size_t alloc_size = sizeof(TexArenaChunk) - TEX_ARENA_CHUNK_SIZE + slab_size;
	TEX_TRACE("Attempting slab malloc of %u bytes... ", (unsigned int)alloc_size);
	TexArenaChunk* c = (TexArenaChunk*)malloc(alloc_size);
	if (!c)
	{
		TEX_TRACE("%s", "FAILED!");
		a->failed = 1;
		return;
	}
	TEX_TRACE("Success. (Ptr: %p)", (void*)c);
	c->next = NULL;
	c->used = 0;
	c->capacity = slab_size;

	a->first = a->current = c;
	a->total_allocated = alloc_size;
}

void arena_reset(TexArena* a)
{
	if (!a || !a->first)
		return;

	TexArenaChunk* c = a->first->next;
	while (c)
	{
		TexArenaChunk* next = c->next;
		free(c);
		c = next;
	}

	a->first->next = NULL;
	a->first->used = 0;
	a->current = a->first;
	a->total_allocated = sizeof(TexArenaChunk) - TEX_ARENA_CHUNK_SIZE + a->first->capacity;
	a->failed = 0;
}

void* arena_alloc(TexArena* a, size_t size, size_t align)
{
	if (!a || a->failed)
		return NULL;
	if (size == 0)
		return NULL;
	size_t max_cap = a->current ? a->current->capacity : TEX_ARENA_CHUNK_SIZE;
	if (max_cap < TEX_ARENA_CHUNK_SIZE)
		max_cap = TEX_ARENA_CHUNK_SIZE;
	if (size > 4096)
	{
		a->failed = 1;
		return NULL;
	}

	TexArenaChunk* c = a->current;
	if (!c)
	{
		a->failed = 1;
		return NULL;
	}

	size_t off = c->used;
	if (align > 1)
	{
		size_t rem = off % align;
		if (rem != 0)
		{
			off += (align - rem);
		}
	}

	if (off + size > c->capacity)
	{
		// allocate a new chunk
		TexArenaChunk* nc = arena_new_chunk(a);
		if (!nc)
		{
			a->failed = 1;
			return NULL;
		}
		c->next = nc;
		a->current = nc;
		a->total_allocated += sizeof(TexArenaChunk) - TEX_ARENA_CHUNK_SIZE + nc->capacity;
		c = nc;

		off = 0;
	}

	void* ptr = c->data + off;
	c->used = off + size;
	return ptr;
}

void arena_free_all(TexArena* a)
{
	if (!a)
		return;
	TexArenaChunk* c = a->first;
	while (c)
	{
		TexArenaChunk* next = c->next;
		free(c);
		c = next;
	}
	memset(a, 0, sizeof(*a));
}
