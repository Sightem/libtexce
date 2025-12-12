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

// 16bit byte offset into the slab for list blocks (allocated in string region, grows down)
typedef uint16_t ListId;
#define LIST_NULL ((ListId)0xFFFF)

// reserved node range for flyweight ASCII glyphs (256 preinitialized nodes)
// NodeRef values 0xFD00-0xFDFF are "reserved" refs that map to static g_reserved_nodes[]
#define TEX_RESERVED_BASE ((NodeRef)0xFD00)
#define TEX_RESERVED_COUNT 256
#define TEX_IS_RESERVED_REF(ref) ((ref) >= TEX_RESERVED_BASE && (ref) < (TEX_RESERVED_BASE + TEX_RESERVED_COUNT))
#define TEX_RESERVED_INDEX(ref) ((ref) - TEX_RESERVED_BASE)

// chunked list block, holds up to 16 NodeRefs, linked to next block
#define TEX_LIST_BLOCK_CAP 16

typedef struct TexListBlock
{
	ListId next; // offset to next block (LIST_NULL if none)
	uint16_t count; // items used in this block (0..TEX_LIST_BLOCK_CAP)
	NodeRef items[TEX_LIST_BLOCK_CAP]; // Node references
} TexListBlock;

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

// alloc one zero initialized list block in string region. returns LIST_NULL on OOM
ListId pool_alloc_list_block(UnifiedPool* pool);


#endif // TEX_TEX_POOL_H
