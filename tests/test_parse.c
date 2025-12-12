#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tex/tex_internal.h"
#include "tex/tex_metrics.h"
#include "tex/tex_parse.h"
#include "tex/tex_pool.h"

// Helper: get the first NodeRef from a ListId (for N_MATH nodes)
static NodeRef list_first_item(UnifiedPool* pool, ListId list_head)
{
	if (list_head == LIST_NULL)
		return NODE_NULL;
	TexListBlock* block = pool_get_list_block(pool, list_head);
	if (!block || block->count == 0)
		return NODE_NULL;
	return block->items[0];
}

static int g_fail = 0;
static void assert_true_int(int cond, const char* msg)
{
	if (!cond)
	{
		fprintf(stderr, "[FAIL] %s\n", msg);
		g_fail++;
	}
}

static void test_frac(void)
{
	TeX_Layout L = { 0 };
	UnifiedPool pool;
	pool_init(&pool, 8192);
	char buf[] = "\\frac{a}{b}";
	NodeRef root_ref = tex_parse_math(buf, (int)strlen(buf), &pool, &L);
	Node* root = pool_get_node(&pool, root_ref);
	assert_true_int(root && root->type == N_MATH, "root is N_MATH");
	Node* f = pool_get_node(&pool, list_first_item(&pool, root->data.list.head));
	assert_true_int(f && f->type == N_FRAC, "frac node present");
	Node* num = pool_get_node(&pool, f->data.frac.num);
	Node* den = pool_get_node(&pool, f->data.frac.den);
	assert_true_int(num && den, "frac has num/den");
	assert_true_int(num->type == N_MATH && list_first_item(&pool, num->data.list.head) != NODE_NULL,
	                "num is group math");
	assert_true_int(den->type == N_MATH && list_first_item(&pool, den->data.list.head) != NODE_NULL,
	                "den is group math");
	Node* num_child = pool_get_node(&pool, list_first_item(&pool, num->data.list.head));
	Node* den_child = pool_get_node(&pool, list_first_item(&pool, den->data.list.head));
	assert_true_int(num_child->type == N_GLYPH && num_child->data.glyph == (unsigned char)'a', "num child 'a'");
	assert_true_int(den_child->type == N_GLYPH && den_child->data.glyph == (unsigned char)'b', "den child 'b'");
	pool_free(&pool);
}

static void test_scripts(void)
{
	TeX_Layout L = { 0 };
	UnifiedPool pool;
	pool_init(&pool, 8192);
	char buf1[] = "x^2";
	NodeRef r1_ref = tex_parse_math(buf1, (int)strlen(buf1), &pool, &L);
	Node* r1 = pool_get_node(&pool, r1_ref);
	Node* n1 = pool_get_node(&pool, list_first_item(&pool, r1->data.list.head));
	assert_true_int(n1 && n1->type == N_SCRIPT, "x^2 -> N_SCRIPT");
	Node* base1 = pool_get_node(&pool, n1->data.script.base);
	Node* sup1 = pool_get_node(&pool, n1->data.script.sup);
	assert_true_int(base1 && sup1, "sup set");
	assert_true_int(base1->data.glyph == (unsigned char)'x', "base 'x'");
	assert_true_int(sup1->type == N_MATH || sup1->type == N_GLYPH, "sup parsed");

	pool_reset(&pool);
	char buf2[] = "x_1^2";
	NodeRef r2_ref = tex_parse_math(buf2, (int)strlen(buf2), &pool, &L);
	Node* r2 = pool_get_node(&pool, r2_ref);
	Node* n2 = pool_get_node(&pool, list_first_item(&pool, r2->data.list.head));
	assert_true_int(n2 && n2->type == N_SCRIPT, "x_1^2 -> N_SCRIPT");
	Node* sub2 = pool_get_node(&pool, n2->data.script.sub);
	Node* sup2 = pool_get_node(&pool, n2->data.script.sup);
	assert_true_int(sub2 && sup2, "sub and sup set");
	pool_free(&pool);
}

static void test_overlays(void)
{
	TeX_Layout L = { 0 };
	UnifiedPool pool;
	pool_init(&pool, 8192);
	char buf1[] = "\\vec{x}";
	NodeRef r1_ref = tex_parse_math(buf1, (int)strlen(buf1), &pool, &L);
	Node* r1 = pool_get_node(&pool, r1_ref);
	Node* n1 = pool_get_node(&pool, list_first_item(&pool, r1->data.list.head));
	assert_true_int(n1 && n1->type == N_OVERLAY, "vec overlay");
	Node* base1 = pool_get_node(&pool, n1->data.overlay.base);
	assert_true_int(base1 && base1->type == N_MATH, "overlay base is group");

	pool_reset(&pool);
	char buf2[] = "\\hat{ab}";
	NodeRef r2_ref = tex_parse_math(buf2, (int)strlen(buf2), &pool, &L);
	Node* r2 = pool_get_node(&pool, r2_ref);
	Node* n2 = pool_get_node(&pool, list_first_item(&pool, r2->data.list.head));
	assert_true_int(n2 && n2->type == N_OVERLAY, "hat overlay");
	Node* base2 = pool_get_node(&pool, n2->data.overlay.base);
	Node* base2_child = pool_get_node(&pool, list_first_item(&pool, base2->data.list.head));
	assert_true_int(base2 && base2_child, "overlay base has child");
	assert_true_int(base2_child->type == N_TEXT, "overlay base child is N_TEXT");
	assert_true_int(base2_child->data.text.len == 2, "overlay base text has len 2");
	pool_free(&pool);
}

static void test_lim_and_bigops(void)
{
	TeX_Layout L = { 0 };
	UnifiedPool pool;
	pool_init(&pool, 8192);
	char buf1[] = "\\lim_{x\\to\\infty}";
	NodeRef r1_ref = tex_parse_math(buf1, (int)strlen(buf1), &pool, &L);
	Node* r1 = pool_get_node(&pool, r1_ref);
	Node* n1 = pool_get_node(&pool, list_first_item(&pool, r1->data.list.head));
	assert_true_int(n1 && n1->type == N_FUNC_LIM, "lim parsed as N_FUNC_LIM");
	assert_true_int(n1->data.func_lim.limit != NODE_NULL, "lim has under limit");

	pool_reset(&pool);
	char buf2[] = "\\sum_{i=0}^n";
	NodeRef r2_ref = tex_parse_math(buf2, (int)strlen(buf2), &pool, &L);
	Node* r2 = pool_get_node(&pool, r2_ref);
	Node* n2 = pool_get_node(&pool, list_first_item(&pool, r2->data.list.head));
	assert_true_int(n2 && n2->type == N_SCRIPT, "sum scripts -> N_SCRIPT");
	Node* base = pool_get_node(&pool, n2->data.script.base);
	Node* sub = pool_get_node(&pool, n2->data.script.sub);
	Node* sup = pool_get_node(&pool, n2->data.script.sup);
	assert_true_int(base && sub && sup, "sum with sub and sup");
	pool_free(&pool);
}

static void test_auto_delim(void)
{
	TeX_Layout L = { 0 };
	UnifiedPool pool;
	pool_init(&pool, 8192);

	// Test basic \left( ... \right)
	char buf1[] = "\\left(x\\right)";
	NodeRef r1_ref = tex_parse_math(buf1, (int)strlen(buf1), &pool, &L);
	Node* r1 = pool_get_node(&pool, r1_ref);
	assert_true_int(r1 && r1->type == N_MATH, "left-right root is N_MATH");
	Node* n1 = pool_get_node(&pool, list_first_item(&pool, r1->data.list.head));
	assert_true_int(n1 && n1->type == N_AUTO_DELIM, "left-right parses to N_AUTO_DELIM");
	assert_true_int(n1->data.auto_delim.left_type == DELIM_PAREN, "left type is DELIM_PAREN");
	assert_true_int(n1->data.auto_delim.right_type == DELIM_PAREN, "right type is DELIM_PAREN");
	assert_true_int(n1->data.auto_delim.content != NODE_NULL, "auto_delim has content");

	// Test \left. ... \right| (mixed delimiters)
	pool_reset(&pool);
	char buf2[] = "\\left.x\\right|";
	NodeRef r2_ref = tex_parse_math(buf2, (int)strlen(buf2), &pool, &L);
	Node* r2 = pool_get_node(&pool, r2_ref);
	Node* n2 = pool_get_node(&pool, list_first_item(&pool, r2->data.list.head));
	assert_true_int(n2 && n2->type == N_AUTO_DELIM, "dot-vert parses to N_AUTO_DELIM");
	assert_true_int(n2->data.auto_delim.left_type == DELIM_NONE, "left type is DELIM_NONE for dot");
	assert_true_int(n2->data.auto_delim.right_type == DELIM_VERT, "right type is DELIM_VERT");

	// Test \left[ ... \right]
	pool_reset(&pool);
	char buf3[] = "\\left[y\\right]";
	NodeRef r3_ref = tex_parse_math(buf3, (int)strlen(buf3), &pool, &L);
	Node* r3 = pool_get_node(&pool, r3_ref);
	Node* n3 = pool_get_node(&pool, list_first_item(&pool, r3->data.list.head));
	assert_true_int(n3 && n3->type == N_AUTO_DELIM, "bracket parses to N_AUTO_DELIM");
	assert_true_int(n3->data.auto_delim.left_type == DELIM_BRACKET, "left type is DELIM_BRACKET");
	assert_true_int(n3->data.auto_delim.right_type == DELIM_BRACKET, "right type is DELIM_BRACKET");

	// Test \left\{ ... \right\} (curly braces)
	pool_reset(&pool);
	char buf4[] = "\\left\\{x\\right\\}";
	NodeRef r4_ref = tex_parse_math(buf4, (int)strlen(buf4), &pool, &L);
	Node* r4 = pool_get_node(&pool, r4_ref);
	assert_true_int(r4 && r4->type == N_MATH, "brace left-right root is N_MATH");
	Node* n4 = pool_get_node(&pool, list_first_item(&pool, r4->data.list.head));
	assert_true_int(n4 && n4->type == N_AUTO_DELIM, "brace parses to N_AUTO_DELIM");
	assert_true_int(n4->data.auto_delim.left_type == DELIM_BRACE, "left type is DELIM_BRACE");
	assert_true_int(n4->data.auto_delim.right_type == DELIM_BRACE, "right type is DELIM_BRACE");

	pool_free(&pool);
}

int main(void)
{
	// Initialize flyweight reserved nodes for ASCII glyphs
	tex_reserved_init();

	test_frac();
	test_scripts();
	test_overlays();
	test_lim_and_bigops();
	test_auto_delim();
	if (g_fail == 0)
	{
		printf("test_parse: PASS\n");
		return 0;
	}
	printf("test_parse: FAIL (%d)\n", g_fail);
	return 1;
}
