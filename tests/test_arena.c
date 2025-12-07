#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "src/tex/tex_arena.h"
#include "tex/tex.h"

static int g_fail = 0;
static void expect(int cond, const char* msg)
{
	if (!cond)
	{
		fprintf(stderr, "[FAIL] %s\n", msg);
		g_fail++;
	}
}

static void test_arena_alloc_basic(void)
{
	TexArena A;
	arena_init(&A);
	void* p1 = arena_alloc(&A, 16, 8);
	void* p2 = arena_alloc(&A, 16, 8);
	expect(p1 && p2, "arena allocations succeeded");
	// Oversized request should fail (> chunk size)
	void* p3 = arena_alloc(&A, TEX_ARENA_CHUNK_SIZE + 1, 8);
	expect(p3 == NULL, "oversized allocation returns NULL");
	expect(A.failed != 0, "arena failed flag set after oversized alloc");
}

static void test_format_free_cycle(void)
{
	TeX_Config cfg = { .color_fg = 1, .color_bg = 255, .font_pack = "TeXFonts" };
	for (int i = 0; i < 3; ++i)
	{
		char buf[] = "a $x^2$ b\ncc dd";
		TeX_Layout* L = tex_format(buf, 64, &cfg);
		expect(L != NULL, "tex_format returned layout");
		expect(tex_get_total_height(L) > 0, "total height positive");
		tex_free(L);
	}
}
int main(void)
{
	test_arena_alloc_basic();
	test_format_free_cycle();
	if (g_fail == 0)
	{
		printf("test_arena: PASS\n");
		return 0;
	}
	printf("test_arena: FAIL (%d)\n", g_fail);
	return 1;
}
