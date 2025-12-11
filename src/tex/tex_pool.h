#ifndef TEX_TEX_POOL_H
#define TEX_TEX_POOL_H

#include <stddef.h>
#include <stdint.h>

struct Node;

// 16bit index into the node array (bottom of slab)
typedef uint16_t NodeRef;
#define NODE_NULL ((NodeRef)0xFFFF)

// 16bit byte offset from the START of the slab to the string data
typedef uint16_t StringId;
#define STRING_NULL ((StringId)0xFFFF)

typedef struct UnifiedPool
{
	uint8_t* slab; // the contiguous memory block
	size_t capacity; // total size in bytes
	size_t node_count; // number of nodes allocated (index cursor)
	size_t string_cursor; // byte offset where free string space begins (grows down)
	size_t peak_used;
	size_t alloc_count;
	size_t reset_count;
} UnifiedPool;

// initialize with a malloc'd buffer of total_size. returns 0 on success, -1 on failure
int pool_init(UnifiedPool* pool, size_t total_size);

// free the internal slab
void pool_free(UnifiedPool* pool);

// reset cursors (does not free memory, allows reuse)
void pool_reset(UnifiedPool* pool);

// allocate one zero-initialized node. returns NODE_NULL on OOM
NodeRef pool_alloc_node(UnifiedPool* pool);

// allocate len bytes + 1 null terminator. returns byte offset ID, or STRING_NULL on OOM
StringId pool_alloc_string(UnifiedPool* pool, const char* src, size_t len);

// get current bytes used in pool (nodes from bottom + strings from top)
size_t pool_get_used(UnifiedPool* pool);


#endif // TEX_TEX_POOL_H
