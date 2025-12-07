#include <stdio.h>
#include <string.h>

#include "tex/tex_internal.h"
#include "tex/tex_measure.h"
#include "tex/tex_parse.h"

static int g_fail = 0;
static void expect(int cond, const char* msg)
{
	if (!cond)
	{
		fprintf(stderr, "[FAIL] %s\n", msg);
		g_fail++;
	}
}

#if TEX_USE_NODE_INDICES
typedef NodeRef NodeHandle;
static Node* hget(TeX_Layout* L, NodeHandle h) { return node_get(&L->pool, h); }
static NodeHandle child_handle(const Node* n) { return n ? n->child : NODE_NULL; }
#else
typedef Node* NodeHandle;
static Node* hget(TeX_Layout* L, NodeHandle h)
{
	(void)L;
	return h;
}
static NodeHandle child_handle(const Node* n) { return n ? n->child : NULL; }
#endif

static Node* first_child(TeX_Layout* L, NodeHandle root_h)
{
	Node* root = hget(L, root_h);
	return root ? hget(L, child_handle(root)) : NULL;
}

static void test_glyph_widths(void)
{
	TeX_Layout L = { 0 };
	pool_init(&L.pool);
	arena_init(&L.arena);
	NodeHandle r1_h = tex_parse_math("x", 1, &L);
	NodeHandle r2_h = tex_parse_math("xx", 2, &L);
	Node* r1 = hget(&L, r1_h);
	Node* r2 = hget(&L, r2_h);
	expect(r1 && r2, "parse roots");
	tex_measure_node(&L, r1, FONTROLE_MAIN);
	tex_measure_node(&L, r2, FONTROLE_MAIN);
	expect(r2->w == 2 * r1->w, "two glyphs width equals 2x single");
}

static void test_fraction_metrics(void)
{
	TeX_Layout L = { 0 };
	pool_init(&L.pool);
	arena_init(&L.arena);
	const char* s = "\\frac{a}{bb}";
	NodeHandle root_h = tex_parse_math((char*)s, (int)strlen(s), &L);
	Node* f = first_child(&L, root_h);
	expect(f && f->type == N_FRAC, "parsed frac");
	tex_measure_node(&L, f, FONTROLE_MAIN);
	Node* num = f->data.frac.num;
	Node* den = f->data.frac.den;
	expect(num && den, "frac has children");
	int inner_w = (num->w > den->w ? num->w : den->w);
	expect(f->w == inner_w + 2 * TEX_FRAC_XPAD + 2 * TEX_FRAC_OUTER_PAD, "frac width = max(num,den)+2*xpad+2*outer");
	expect(f->asc > 0 && f->desc > 0, "frac asc/desc positive");
}

static void test_scripts_metrics(void)
{
	TeX_Layout L = { 0 };
	pool_init(&L.pool);
	arena_init(&L.arena);
	const char* s = "x_1^2";
	NodeHandle root_h = tex_parse_math((char*)s, (int)strlen(s), &L);
	Node* sc = first_child(&L, root_h);
	expect(sc && sc->type == N_SCRIPT, "parsed script");
	tex_measure_node(&L, sc, FONTROLE_MAIN);
	Node* base = sc->data.script.base;
	Node* sub = sc->data.script.sub;
	Node* sup = sc->data.script.sup;
	int w_scripts = 0;
	if (sub && sub->w > w_scripts)
		w_scripts = sub->w;
	if (sup && sup->w > w_scripts)
		w_scripts = sup->w;
	expect(sc->w == base->w + TEX_SCRIPT_XPAD + w_scripts, "script width = base + pad + max(sub/sup)");
	expect(sc->asc >= base->asc, "script asc >= base asc");
	expect(sc->desc >= base->desc, "script desc >= base desc");
}

static void test_sqrt_metrics(void)
{
	TeX_Layout L = { 0 };
	pool_init(&L.pool);
	arena_init(&L.arena);
	const char* s = "\\sqrt{xx}";
	NodeHandle root_h = tex_parse_math((char*)s, (int)strlen(s), &L);
	Node* sq = first_child(&L, root_h);
	expect(sq && sq->type == N_SQRT, "parsed sqrt");
	tex_measure_node(&L, sq, FONTROLE_MAIN);
	Node* rad = sq->data.sqrt.rad;
	expect(sq->w > (rad ? rad->w : 0), "sqrt width > radicand width");
}

static void test_lim_metrics(void)
{
	TeX_Layout L = { 0 };
	pool_init(&L.pool);
	arena_init(&L.arena);
	const char* s = "\\lim_{n\\to\\infty}";
	NodeHandle root_h = tex_parse_math((char*)s, (int)strlen(s), &L);
	Node* lm = first_child(&L, root_h);
	expect(lm && lm->type == N_FUNC_LIM, "parsed lim");
	tex_measure_node(&L, lm, FONTROLE_MAIN);
	Node* lim = lm->data.func_lim.limit;
	expect(lm->w >= (lim ? lim->w : 0), "lim width >= under-limit width");
	expect(lm->desc > 0, "lim desc positive due to under-limit");
}

int main(void)
{
	test_glyph_widths();
	test_fraction_metrics();
	test_scripts_metrics();
	test_sqrt_metrics();
	test_lim_metrics();
	if (g_fail == 0)
	{
		printf("test_measure: PASS\n");
		return 0;
	}
	printf("test_measure: FAIL (%d)\n", g_fail);
	return 1;
}
