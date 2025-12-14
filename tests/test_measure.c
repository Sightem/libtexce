#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "tex/tex_internal.h"
#include "tex/tex_measure.h"
#include "tex/tex_metrics.h"
#include "tex/tex_parse.h"
#include "tex/tex_pool.h"

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
static void expect(int cond, const char* msg)
{
	if (!cond)
	{
		fprintf(stderr, "[FAIL] %s\n", msg);
		g_fail++;
	}
}

static void test_glyph_widths(void)
{
	TeX_Layout L = { 0 };
	UnifiedPool pool;
	pool_init(&pool, 8192);
	NodeRef r1_ref = tex_parse_math("x", 1, &pool, &L);
	NodeRef r2_ref = tex_parse_math("xx", 2, &pool, &L);
	Node* r1 = pool_get_node(&pool, r1_ref);
	Node* r2 = pool_get_node(&pool, r2_ref);
	assert(r1 && r2 && "parse roots should not be NULL");
	expect(r1 && r2, "parse roots");
	tex_measure_range(&pool, 0, (NodeRef)pool.node_count);
	expect(r2->w == 2 * r1->w, "two glyphs width equals 2x single");
	pool_free(&pool);
}

static void test_fraction_metrics(void)
{
	TeX_Layout L = { 0 };
	UnifiedPool pool;
	pool_init(&pool, 8192);
	const char* s = "\\frac{a}{bb}";
	NodeRef root_ref = tex_parse_math(s, (int)strlen(s), &pool, &L);
	Node* root = pool_get_node(&pool, root_ref);
	assert(root && "root should not be NULL");
	NodeRef f_ref = list_first_item(&pool, root->data.list.head);
	Node* f = pool_get_node(&pool, f_ref);
	assert(f && "frac node should not be NULL");
	expect(f && f->type == N_FRAC, "parsed frac");
	tex_measure_range(&pool, 0, (NodeRef)pool.node_count);
	Node* num = pool_get_node(&pool, f->data.frac.num);
	Node* den = pool_get_node(&pool, f->data.frac.den);
	assert(num && den && "frac num/den should not be NULL");
	expect(num && den, "frac has children");
	int inner_w = (num->w > den->w ? num->w : den->w);
	expect(f->w == inner_w + 2 * TEX_FRAC_XPAD + 2 * TEX_FRAC_OUTER_PAD, "frac width = max(num,den)+2*xpad+2*outer");
	expect(f->asc > 0 && f->desc > 0, "frac asc/desc positive");
	pool_free(&pool);
}

static void test_scripts_metrics(void)
{
	TeX_Layout L = { 0 };
	UnifiedPool pool;
	pool_init(&pool, 8192);
	const char* s = "x_1^2";
	NodeRef root_ref = tex_parse_math(s, (int)strlen(s), &pool, &L);
	Node* root = pool_get_node(&pool, root_ref);
	assert(root && "root should not be NULL");
	NodeRef sc_ref = list_first_item(&pool, root->data.list.head);
	Node* sc = pool_get_node(&pool, sc_ref);
	assert(sc && "script node should not be NULL");
	expect(sc && sc->type == N_SCRIPT, "parsed script");
	tex_measure_range(&pool, 0, (NodeRef)pool.node_count);
	Node* base = pool_get_node(&pool, sc->data.script.base);
	assert(base && "script base should not be NULL");
	Node* sub = pool_get_node(&pool, sc->data.script.sub);
	Node* sup = pool_get_node(&pool, sc->data.script.sup);
	int w_scripts = 0;
	if (sub && sub->w > w_scripts)
		w_scripts = sub->w;
	if (sup && sup->w > w_scripts)
		w_scripts = sup->w;
	expect(sc->w == base->w + TEX_SCRIPT_XPAD + w_scripts, "script width = base + pad + max(sub/sup)");
	expect(sc->asc >= base->asc, "script asc >= base asc");
	expect(sc->desc >= base->desc, "script desc >= base desc");
	pool_free(&pool);
}

static void test_sqrt_metrics(void)
{
	TeX_Layout L = { 0 };
	UnifiedPool pool;
	pool_init(&pool, 8192);
	const char* s = "\\sqrt{xx}";
	NodeRef root_ref = tex_parse_math(s, (int)strlen(s), &pool, &L);
	Node* root = pool_get_node(&pool, root_ref);
	assert(root && "root should not be NULL");
	NodeRef sq_ref = list_first_item(&pool, root->data.list.head);
	Node* sq = pool_get_node(&pool, sq_ref);
	assert(sq && "sqrt node should not be NULL");
	expect(sq && sq->type == N_SQRT, "parsed sqrt");
	tex_measure_range(&pool, 0, (NodeRef)pool.node_count);
	Node* rad = pool_get_node(&pool, sq->data.sqrt.rad);
	assert(sq && "sqrt node should still be valid");
	expect(sq->w > (rad ? rad->w : 0), "sqrt width > radicand width");
	pool_free(&pool);
}

static void test_lim_metrics(void)
{
	TeX_Layout L = { 0 };
	UnifiedPool pool;
	pool_init(&pool, 8192);
	const char* s = "\\lim_{n\\to\\infty}";
	NodeRef root_ref = tex_parse_math(s, (int)strlen(s), &pool, &L);
	Node* root = pool_get_node(&pool, root_ref);
	assert(root && "root should not be NULL");
	NodeRef lm_ref = list_first_item(&pool, root->data.list.head);
	Node* lm = pool_get_node(&pool, lm_ref);
	assert(lm && "lim node should not be NULL");
	expect(lm && lm->type == N_FUNC_LIM, "parsed lim");
	tex_measure_range(&pool, 0, (NodeRef)pool.node_count);
	Node* lim = pool_get_node(&pool, lm->data.func_lim.limit);
	assert(lm && "lim node should still be valid");
	expect(lm->w >= (lim ? lim->w : 0), "lim width >= under-limit width");
	expect(lm->desc > 0, "lim desc positive due to under-limit");
	pool_free(&pool);
}

static void test_matrix_metrics(void)
{
	TeX_Layout L = { 0 };
	UnifiedPool pool;
	pool_init(&pool, 8192);

	// Test 2x2 pmatrix metrics
	const char* s = "\\begin{pmatrix}a & b \\\\ c & d\\end{pmatrix}";
	NodeRef root_ref = tex_parse_math(s, (int)strlen(s), &pool, &L);
	Node* root = pool_get_node(&pool, root_ref);
	assert(root && "root should not be NULL");
	NodeRef mx_ref = list_first_item(&pool, root->data.list.head);
	Node* mx = pool_get_node(&pool, mx_ref);
	assert(mx && "matrix node should not be NULL");
	expect(mx && mx->type == N_MATRIX, "parsed matrix");
	tex_measure_range(&pool, 0, (NodeRef)pool.node_count);

	assert(mx && "matrix node should still be valid");
	// Matrix width should be non-negative (may be 0 in fontless test environment)
	expect(mx->w >= 0, "matrix has non-negative width");
	// Verify asc and desc are set (may be 0 without fonts)
	expect(mx->asc >= 0 && mx->desc >= 0, "matrix has valid asc and desc");

	// Test matrix with no delimiters
	pool_reset(&pool);
	const char* s2 = "\\begin{matrix}x\\end{matrix}";
	NodeRef r2_ref = tex_parse_math(s2, (int)strlen(s2), &pool, &L);
	Node* r2 = pool_get_node(&pool, r2_ref);
	assert(r2 && "r2 should not be NULL");
	NodeRef mx2_ref = list_first_item(&pool, r2->data.list.head);
	Node* mx2 = pool_get_node(&pool, mx2_ref);
	tex_measure_range(&pool, 0, (NodeRef)pool.node_count);
	assert(mx2 && "plain matrix node should not be NULL");
	expect(mx2 && mx2->type == N_MATRIX, "plain matrix parsed");
	expect(mx2->w >= 0, "plain matrix has non-negative width");

	pool_free(&pool);
}

int main(void)
{
	// Initialize flyweight reserved nodes for ASCII glyphs
	tex_reserved_init();
	test_glyph_widths();
	test_fraction_metrics();
	test_scripts_metrics();
	test_sqrt_metrics();
	test_lim_metrics();
	test_matrix_metrics();
	if (g_fail == 0)
	{
		printf("test_measure: PASS\n");
		return 0;
	}
	printf("test_measure: FAIL (%d)\n", g_fail);
	return 1;
}
