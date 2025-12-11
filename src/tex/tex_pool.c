#include "tex_pool.h"
#include <stdlib.h>
#include <string.h>
#include "tex_internal.h"

static void update_peak(UnifiedPool* pool)
{
	size_t used = pool_get_used(pool);
	if (used > pool->peak_used)
		pool->peak_used = used;
}

int pool_init(UnifiedPool* pool, size_t total_size)
{
	if (!pool || total_size == 0)
		return -1;

	pool->slab = (uint8_t*)malloc(total_size);
	if (!pool->slab)
		return -1;

	pool->capacity = total_size;
	pool->peak_used = 0;
	pool->alloc_count = 0;
	pool->reset_count = 0;
	pool_reset(pool);
	return 0;
}

void pool_free(UnifiedPool* pool)
{
	if (pool && pool->slab)
	{
		free(pool->slab);
		pool->slab = NULL;
		pool->capacity = 0;
		pool->node_count = 0;
		pool->string_cursor = 0;
	}
}

void pool_reset(UnifiedPool* pool)
{
	if (pool)
	{
		pool->node_count = 0;
		pool->string_cursor = pool->capacity;
		pool->reset_count++;
	}
}

size_t pool_get_used(UnifiedPool* pool)
{
	if (!pool)
		return 0;
	size_t nodes_used = pool->node_count * sizeof(Node);
	size_t strings_used = pool->capacity - pool->string_cursor;
	return nodes_used + strings_used;
}

NodeRef pool_alloc_node(UnifiedPool* pool)
{
	if (!pool || !pool->slab)
		return NODE_NULL;

	size_t node_size = sizeof(Node);
	size_t current_node_end = pool->node_count * node_size;
	size_t next_node_end = current_node_end + node_size;

	// would overlap with strings?
	if (next_node_end > pool->string_cursor)
		return NODE_NULL;

	// (uint16_t max - 1, since NODE_NULL = 0xFFFF)
	if (pool->node_count >= 0xFFFE)
		return NODE_NULL;

	NodeRef ref = (NodeRef)pool->node_count;
	pool->node_count++;
	pool->alloc_count++;

	Node* ptr = (Node*)(pool->slab + current_node_end);
	memset(ptr, 0, node_size); // NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
	ptr->next = NODE_NULL;
	ptr->child = NODE_NULL;

	update_peak(pool);
	return ref;
}

StringId pool_alloc_string(UnifiedPool* pool, const char* src, size_t len)
{
	if (!pool || !pool->slab || !src)
		return STRING_NULL;

	size_t size_needed = len + 1; // + null terminator
	size_t node_boundary = pool->node_count * sizeof(Node);

	// would overlap with nodes?
	if (pool->string_cursor < size_needed || (pool->string_cursor - size_needed) < node_boundary)
	{
		return STRING_NULL;
	}

	if (pool->string_cursor - size_needed > 0xFFFE)
		return STRING_NULL;

	// alloc downward
	pool->string_cursor -= size_needed;
	pool->alloc_count++;

	char* dst = (char*)(pool->slab + pool->string_cursor);
	memcpy(dst, src, len); // NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
	dst[len] = '\0';

	update_peak(pool);
	return (StringId)pool->string_cursor;
}
