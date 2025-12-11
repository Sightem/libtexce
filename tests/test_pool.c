#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tex/tex_internal.h"
#include "tex/tex_pool.h"

static int g_fail = 0;

static void expect(int cond, const char* msg)
{
	if (!cond)
	{
		fprintf(stderr, "[FAIL] %s\n", msg);
		g_fail++;
	}
}

static void test_pool_basic_alloc(void)
{
	UnifiedPool pool;
	int rc = pool_init(&pool, 4096);
	expect(rc == 0, "pool_init succeeds");
	expect(pool.slab != NULL, "slab allocated");
	expect(pool.capacity == 4096, "capacity correct");
	expect(pool.node_count == 0, "node_count starts at 0");
	expect(pool.string_cursor == 4096, "string_cursor starts at capacity");

	// Allocate a node
	NodeRef n1 = pool_alloc_node(&pool);
	expect(n1 == 0, "first node has ref 0");
	expect(pool.node_count == 1, "node_count incremented");

	Node* ptr1 = pool_get_node(&pool, n1);
	expect(ptr1 != NULL, "pool_get_node returns valid pointer");
	expect(ptr1->next == NODE_NULL, "node.next initialized to NODE_NULL");
	expect(ptr1->child == NODE_NULL, "node.child initialized to NODE_NULL");

	// Allocate another node
	NodeRef n2 = pool_alloc_node(&pool);
	expect(n2 == 1, "second node has ref 1");

	pool_free(&pool);
}

static void test_pool_string_alloc(void)
{
	UnifiedPool pool;
	pool_init(&pool, 1024);

	StringId s1 = pool_alloc_string(&pool, "Hello", 5);
	expect(s1 != STRING_NULL, "string allocation succeeds");
	// string_cursor should be at 1024 - 6 = 1018
	expect(s1 == 1018, "string allocated at correct offset");
	expect(pool.string_cursor == 1018, "string_cursor updated");

	const char* str = pool_get_string(&pool, s1);
	expect(strcmp(str, "Hello") == 0, "string content matches");

	// Allocate another string
	StringId s2 = pool_alloc_string(&pool, "World", 5);
	expect(s2 == 1012, "second string at correct offset");

	pool_free(&pool);
}

static void test_pool_collision(void)
{
	UnifiedPool pool;
	// Small pool: only enough for ~2 nodes and a short string
	size_t node_size = sizeof(Node);
	size_t pool_size = node_size * 2 + 20; // 2 nodes + some string space
	pool_init(&pool, pool_size);

	// Allocate 2 nodes
	NodeRef n1 = pool_alloc_node(&pool);
	NodeRef n2 = pool_alloc_node(&pool);
	expect(n1 != NODE_NULL && n2 != NODE_NULL, "allocate 2 nodes");

	// Now nodes take up 2*sizeof(Node) from bottom
	// Allocate a string that almost fills remaining space
	size_t remaining = pool.string_cursor - (pool.node_count * node_size);
	// Try to allocate string that exactly fits
	char* test_str = (char*)malloc(remaining);
	memset(test_str, 'x', remaining - 1);
	test_str[remaining - 1] = '\0';
	StringId s1 = pool_alloc_string(&pool, test_str, remaining - 1);
	expect(s1 != STRING_NULL, "string fits in remaining space");

	// Now pool should be full - no room for another node
	NodeRef n3 = pool_alloc_node(&pool);
	expect(n3 == NODE_NULL, "node allocation fails when pool full");

	free(test_str);
	pool_free(&pool);
}

static void test_pool_reset(void)
{
	UnifiedPool pool;
	pool_init(&pool, 2048);

	// Allocate some stuff
	pool_alloc_node(&pool);
	pool_alloc_node(&pool);
	pool_alloc_string(&pool, "test", 4);

	expect(pool.node_count == 2, "nodes allocated before reset");
	expect(pool.string_cursor < 2048, "string_cursor moved before reset");

	// Reset
	pool_reset(&pool);

	expect(pool.node_count == 0, "node_count reset to 0");
	expect(pool.string_cursor == 2048, "string_cursor reset to capacity");
	expect(pool.slab != NULL, "slab still valid after reset");

	// Should be able to allocate again
	NodeRef n = pool_alloc_node(&pool);
	expect(n == 0, "can allocate after reset");

	pool_free(&pool);
}

static void test_pool_invalid_access(void)
{
	UnifiedPool pool;
	pool_init(&pool, 1024);

	// Invalid refs should return NULL/empty
	Node* bad = pool_get_node(&pool, NODE_NULL);
	expect(bad == NULL, "NODE_NULL returns NULL pointer");

	const char* empty = pool_get_string(&pool, STRING_NULL);
	expect(empty != NULL && empty[0] == '\0', "STRING_NULL returns empty string");

	pool_free(&pool);
}

int main(void)
{
	test_pool_basic_alloc();
	test_pool_string_alloc();
	test_pool_collision();
	test_pool_reset();
	test_pool_invalid_access();

	if (g_fail == 0)
	{
		printf("test_pool: PASS\n");
		return 0;
	}
	printf("test_pool: FAIL (%d)\n", g_fail);
	return 1;
}
