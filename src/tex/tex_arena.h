#ifndef TEX_TEX_ARENA_H
#define TEX_TEX_ARENA_H

#include <stddef.h>
#include <stdint.h>

#ifndef TEX_ARENA_CHUNK_SIZE
#define TEX_ARENA_CHUNK_SIZE 2048
#endif

typedef struct TexArenaChunk
{
	struct TexArenaChunk* next;
	size_t used;
	size_t capacity; // total usable bytes in data[] for this chunk
	uint8_t data[TEX_ARENA_CHUNK_SIZE];
} TexArenaChunk;

typedef struct
{
	TexArenaChunk* first;
	TexArenaChunk* current;
	size_t total_allocated; // bytes reserved across chunks (diagnostic)
	int failed;
} TexArena;

void arena_init(TexArena* a);

// Initialize arena with a single fixed-size slab (no chunk growth for render slab)
void arena_init_slab(TexArena* a, size_t slab_size);

// Reset arena to initial state (resets offset, keeps first chunk, frees extras)
void arena_reset(TexArena* a);

void* arena_alloc(TexArena* a, size_t size, size_t align);

// Free all chunks and reset arena
void arena_free_all(TexArena* a);

static inline char* arena_strndup(TexArena* a, const char* s, size_t len)
{
	if (!a || a->failed)
	{
		return NULL;
	}
	size_t n = len + 1;
	char* p = (char*)arena_alloc(a, n, sizeof(void*));
	if (!p)
	{
		return NULL;
	}
	if (s && len)
	{
		for (size_t i = 0; i < len; ++i)
		{
			p[i] = s[i];
		}
	}
	p[len] = '\0';
	return p;
}

static inline char* arena_strdup(TexArena* a, const char* s)
{
	size_t len = 0;
	if (s)
	{
		while (s[len])
		{
			++len;
		}
	}
	return arena_strndup(a, s, len);
}

#endif // TEX_TEX_ARENA_H
