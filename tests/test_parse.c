#include <assert.h>
#include <stdio.h>
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
	assert(root && "root should not be NULL");
	assert_true_int(root && root->type == N_MATH, "root is N_MATH");
	Node* f = pool_get_node(&pool, list_first_item(&pool, root->data.list.head));
	assert(f && "frac node should not be NULL");
	assert_true_int(f && f->type == N_FRAC, "frac node present");
	Node* num = pool_get_node(&pool, f->data.frac.num);
	Node* den = pool_get_node(&pool, f->data.frac.den);
	assert(num && den && "frac num/den should not be NULL");
	assert_true_int(num && den, "frac has num/den");
	assert_true_int(num->type == N_MATH && list_first_item(&pool, num->data.list.head) != NODE_NULL,
	                "num is group math");
	assert_true_int(den->type == N_MATH && list_first_item(&pool, den->data.list.head) != NODE_NULL,
	                "den is group math");
	Node* num_child = pool_get_node(&pool, list_first_item(&pool, num->data.list.head));
	Node* den_child = pool_get_node(&pool, list_first_item(&pool, den->data.list.head));
	assert(num_child && den_child && "num/den children should not be NULL");
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
	assert(n1 && "script node should not be NULL");
	assert_true_int(n1 && n1->type == N_SCRIPT, "x^2 -> N_SCRIPT");
	Node* base1 = pool_get_node(&pool, n1->data.script.base);
	assert(base1 && "script base should not be NULL");
	Node* sup1 = pool_get_node(&pool, n1->data.script.sup);
	assert_true_int(base1 && sup1, "sup set");
	assert_true_int(base1->data.glyph == (unsigned char)'x', "base 'x'");
	assert_true_int(sup1->type == N_MATH || sup1->type == N_GLYPH, "sup parsed");

	pool_reset(&pool);
	char buf2[] = "x_1^2";
	NodeRef r2_ref = tex_parse_math(buf2, (int)strlen(buf2), &pool, &L);
	Node* r2 = pool_get_node(&pool, r2_ref);
	assert(r2 && "r2 should not be NULL");
	Node* n2 = pool_get_node(&pool, list_first_item(&pool, r2->data.list.head));
	assert(n2 && "n2 should not be NULL");
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
	assert(r1 && "r1 should not be NULL");
	Node* n1 = pool_get_node(&pool, list_first_item(&pool, r1->data.list.head));
	assert(n1 && "n1 should not be NULL");
	assert_true_int(n1 && n1->type == N_OVERLAY, "vec overlay");
	Node* base1 = pool_get_node(&pool, n1->data.overlay.base);
	assert(base1 && "base1 should not be NULL");
	assert_true_int(base1 && base1->type == N_MATH, "overlay base is group");

	pool_reset(&pool);
	char buf2[] = "\\hat{ab}";
	NodeRef r2_ref = tex_parse_math(buf2, (int)strlen(buf2), &pool, &L);
	Node* r2 = pool_get_node(&pool, r2_ref);
	assert(r2 && "r2 should not be NULL");
	Node* n2 = pool_get_node(&pool, list_first_item(&pool, r2->data.list.head));
	assert(n2 && "n2 should not be NULL");
	assert_true_int(n2 && n2->type == N_OVERLAY, "hat overlay");
	Node* base2 = pool_get_node(&pool, n2->data.overlay.base);
	assert(base2 && "base2 should not be NULL");
	Node* base2_child = pool_get_node(&pool, list_first_item(&pool, base2->data.list.head));
	assert(base2_child && "base2_child should not be NULL");
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
	assert(r1 && "r1 should not be NULL");
	Node* n1 = pool_get_node(&pool, list_first_item(&pool, r1->data.list.head));
	assert(n1 && "n1 should not be NULL");
	assert_true_int(n1 && n1->type == N_FUNC_LIM, "lim parsed as N_FUNC_LIM");
	assert_true_int(n1->data.func_lim.limit != NODE_NULL, "lim has under limit");

	pool_reset(&pool);
	char buf2[] = "\\sum_{i=0}^n";
	NodeRef r2_ref = tex_parse_math(buf2, (int)strlen(buf2), &pool, &L);
	Node* r2 = pool_get_node(&pool, r2_ref);
	assert(r2 && "r2 should not be NULL");
	Node* n2 = pool_get_node(&pool, list_first_item(&pool, r2->data.list.head));
	assert(n2 && "n2 should not be NULL");
	assert_true_int(n2 && n2->type == N_SCRIPT, "sum scripts -> N_SCRIPT");
	Node* base = pool_get_node(&pool, n2->data.script.base);
	Node* sub_node = pool_get_node(&pool, n2->data.script.sub);
	Node* sup_node = pool_get_node(&pool, n2->data.script.sup);
	assert_true_int(base && sub_node && sup_node, "sum with sub and sup");
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
	assert(r1 && "r1 should not be NULL");
	assert_true_int(r1 && r1->type == N_MATH, "left-right root is N_MATH");
	Node* n1 = pool_get_node(&pool, list_first_item(&pool, r1->data.list.head));
	assert(n1 && "n1 should not be NULL");
	assert_true_int(n1 && n1->type == N_AUTO_DELIM, "left-right parses to N_AUTO_DELIM");
	assert_true_int(n1->data.auto_delim.left_type == DELIM_PAREN, "left type is DELIM_PAREN");
	assert_true_int(n1->data.auto_delim.right_type == DELIM_PAREN, "right type is DELIM_PAREN");
	assert_true_int(n1->data.auto_delim.content != NODE_NULL, "auto_delim has content");

	// Test \left. ... \right| (mixed delimiters)
	pool_reset(&pool);
	char buf2[] = "\\left.x\\right|";
	NodeRef r2_ref = tex_parse_math(buf2, (int)strlen(buf2), &pool, &L);
	Node* r2 = pool_get_node(&pool, r2_ref);
	assert(r2 && "r2 should not be NULL");
	Node* n2 = pool_get_node(&pool, list_first_item(&pool, r2->data.list.head));
	assert(n2 && "n2 should not be NULL");
	assert_true_int(n2 && n2->type == N_AUTO_DELIM, "dot-vert parses to N_AUTO_DELIM");
	assert_true_int(n2->data.auto_delim.left_type == DELIM_NONE, "left type is DELIM_NONE for dot");
	assert_true_int(n2->data.auto_delim.right_type == DELIM_VERT, "right type is DELIM_VERT");

	// Test \left[ ... \right]
	pool_reset(&pool);
	char buf3[] = "\\left[y\\right]";
	NodeRef r3_ref = tex_parse_math(buf3, (int)strlen(buf3), &pool, &L);
	Node* r3 = pool_get_node(&pool, r3_ref);
	assert(r3 && "r3 should not be NULL");
	Node* n3 = pool_get_node(&pool, list_first_item(&pool, r3->data.list.head));
	assert(n3 && "n3 should not be NULL");
	assert_true_int(n3 && n3->type == N_AUTO_DELIM, "bracket parses to N_AUTO_DELIM");
	assert_true_int(n3->data.auto_delim.left_type == DELIM_BRACKET, "left type is DELIM_BRACKET");
	assert_true_int(n3->data.auto_delim.right_type == DELIM_BRACKET, "right type is DELIM_BRACKET");

	// Test \left\{ ... \right\} (curly braces)
	pool_reset(&pool);
	char buf4[] = "\\left\\{x\\right\\}";
	NodeRef r4_ref = tex_parse_math(buf4, (int)strlen(buf4), &pool, &L);
	Node* r4 = pool_get_node(&pool, r4_ref);
	assert(r4 && "r4 should not be NULL");
	assert_true_int(r4 && r4->type == N_MATH, "brace left-right root is N_MATH");
	Node* n4 = pool_get_node(&pool, list_first_item(&pool, r4->data.list.head));
	assert(n4 && "n4 should not be NULL");
	assert_true_int(n4 && n4->type == N_AUTO_DELIM, "brace parses to N_AUTO_DELIM");
	assert_true_int(n4->data.auto_delim.left_type == DELIM_BRACE, "left type is DELIM_BRACE");
	assert_true_int(n4->data.auto_delim.right_type == DELIM_BRACE, "right type is DELIM_BRACE");

	pool_free(&pool);
}

static void test_matrix(void)
{
	TeX_Layout L = { 0 };
	UnifiedPool pool;
	pool_init(&pool, 8192);

	// Test basic pmatrix with 2x2 cells
	char buf1[] = "\\begin{pmatrix}a & b \\\\ c & d\\end{pmatrix}";
	NodeRef r1_ref = tex_parse_math(buf1, (int)strlen(buf1), &pool, &L);
	Node* r1 = pool_get_node(&pool, r1_ref);
	assert(r1 && "r1 should not be NULL");
	assert_true_int(r1 && r1->type == N_MATH, "pmatrix root is N_MATH");
	Node* m1 = pool_get_node(&pool, list_first_item(&pool, r1->data.list.head));
	assert(m1 && "m1 should not be NULL");
	assert_true_int(m1 && m1->type == N_MATRIX, "pmatrix parses to N_MATRIX");
	assert_true_int(m1->data.matrix.rows == 2, "pmatrix has 2 rows");
	assert_true_int(m1->data.matrix.cols == 2, "pmatrix has 2 cols");
	assert_true_int(m1->data.matrix.delim_type == DELIM_PAREN, "pmatrix delim_type is DELIM_PAREN");

	// Test bmatrix
	pool_reset(&pool);
	char buf2[] = "\\begin{bmatrix}1 & 0 \\\\ 0 & 1\\end{bmatrix}";
	NodeRef r2_ref = tex_parse_math(buf2, (int)strlen(buf2), &pool, &L);
	Node* r2 = pool_get_node(&pool, r2_ref);
	assert(r2 && "r2 should not be NULL");
	Node* m2 = pool_get_node(&pool, list_first_item(&pool, r2->data.list.head));
	assert(m2 && "m2 should not be NULL");
	assert_true_int(m2 && m2->type == N_MATRIX, "bmatrix parses to N_MATRIX");
	assert_true_int(m2->data.matrix.delim_type == DELIM_BRACKET, "bmatrix delim_type is DELIM_BRACKET");

	// Test Bmatrix (curly braces)
	pool_reset(&pool);
	char buf3[] = "\\begin{Bmatrix}x \\\\ y\\end{Bmatrix}";
	NodeRef r3_ref = tex_parse_math(buf3, (int)strlen(buf3), &pool, &L);
	Node* r3 = pool_get_node(&pool, r3_ref);
	assert(r3 && "r3 should not be NULL");
	Node* m3 = pool_get_node(&pool, list_first_item(&pool, r3->data.list.head));
	assert(m3 && "m3 should not be NULL");
	assert_true_int(m3 && m3->type == N_MATRIX, "Bmatrix parses to N_MATRIX");
	assert_true_int(m3->data.matrix.delim_type == DELIM_BRACE, "Bmatrix delim_type is DELIM_BRACE");
	assert_true_int(m3->data.matrix.rows == 2, "Bmatrix column vector has 2 rows");
	assert_true_int(m3->data.matrix.cols == 1, "Bmatrix column vector has 1 col");

	// Test vmatrix
	pool_reset(&pool);
	char buf4[] = "\\begin{vmatrix}a\\end{vmatrix}";
	NodeRef r4_ref = tex_parse_math(buf4, (int)strlen(buf4), &pool, &L);
	Node* r4 = pool_get_node(&pool, r4_ref);
	assert(r4 && "r4 should not be NULL");
	Node* m4 = pool_get_node(&pool, list_first_item(&pool, r4->data.list.head));
	assert(m4 && "m4 should not be NULL");
	assert_true_int(m4 && m4->type == N_MATRIX, "vmatrix parses to N_MATRIX");
	assert_true_int(m4->data.matrix.delim_type == DELIM_VERT, "vmatrix delim_type is DELIM_VERT");

	// Test plain matrix (no delimiters)
	pool_reset(&pool);
	char buf5[] = "\\begin{matrix}1 & 2 & 3\\end{matrix}";
	NodeRef r5_ref = tex_parse_math(buf5, (int)strlen(buf5), &pool, &L);
	Node* r5 = pool_get_node(&pool, r5_ref);
	assert(r5 && "r5 should not be NULL");
	Node* m5 = pool_get_node(&pool, list_first_item(&pool, r5->data.list.head));
	assert(m5 && "m5 should not be NULL");
	assert_true_int(m5 && m5->type == N_MATRIX, "matrix parses to N_MATRIX");
	assert_true_int(m5->data.matrix.delim_type == DELIM_NONE, "matrix delim_type is DELIM_NONE");
	assert_true_int(m5->data.matrix.rows == 1, "single-row matrix has 1 row");
	assert_true_int(m5->data.matrix.cols == 3, "3-element row has 3 cols");

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
	test_matrix();
	if (g_fail == 0)
	{
		printf("test_parse: PASS\n");
		return 0;
	}
	printf("test_parse: FAIL (%d)\n", g_fail);
	return 1;
}
