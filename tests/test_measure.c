#include <stdio.h>
#include <string.h>

#include "tex/tex_arena.h"
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

static Node* first_child(Node* root) { return root ? root->child : NULL; }

static void test_glyph_widths(void)
{
	TeX_Layout L = { 0 };
	TexArena arena;
	arena_init(&arena);
	Node* r1 = tex_parse_math("x", 1, &arena, &L);
	Node* r2 = tex_parse_math("xx", 2, &arena, &L);
	expect(r1 && r2, "parse roots");
	tex_measure_node(r1, FONTROLE_MAIN);
	tex_measure_node(r2, FONTROLE_MAIN);
	expect(r2->w == 2 * r1->w, "two glyphs width equals 2x single");
	arena_free_all(&arena);
}

static void test_fraction_metrics(void)
{
	TeX_Layout L = { 0 };
	TexArena arena;
	arena_init(&arena);
	const char* s = "\\frac{a}{bb}";
	Node* root = tex_parse_math(s, (int)strlen(s), &arena, &L);
	Node* f = first_child(root);
	expect(f && f->type == N_FRAC, "parsed frac");
	tex_measure_node(f, FONTROLE_MAIN);
	Node* num = f->data.frac.num;
	Node* den = f->data.frac.den;
	expect(num && den, "frac has children");
	int inner_w = (num->w > den->w ? num->w : den->w);
	expect(f->w == inner_w + 2 * TEX_FRAC_XPAD + 2 * TEX_FRAC_OUTER_PAD, "frac width = max(num,den)+2*xpad+2*outer");
	expect(f->asc > 0 && f->desc > 0, "frac asc/desc positive");
	arena_free_all(&arena);
}

static void test_scripts_metrics(void)
{
	TeX_Layout L = { 0 };
	TexArena arena;
	arena_init(&arena);
	const char* s = "x_1^2";
	Node* root = tex_parse_math(s, (int)strlen(s), &arena, &L);
	Node* sc = first_child(root);
	expect(sc && sc->type == N_SCRIPT, "parsed script");
	tex_measure_node(sc, FONTROLE_MAIN);
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
	arena_free_all(&arena);
}

static void test_sqrt_metrics(void)
{
	TeX_Layout L = { 0 };
	TexArena arena;
	arena_init(&arena);
	const char* s = "\\sqrt{xx}";
	Node* root = tex_parse_math(s, (int)strlen(s), &arena, &L);
	Node* sq = first_child(root);
	expect(sq && sq->type == N_SQRT, "parsed sqrt");
	tex_measure_node(sq, FONTROLE_MAIN);
	Node* rad = sq->data.sqrt.rad;
	expect(sq->w > (rad ? rad->w : 0), "sqrt width > radicand width");
	arena_free_all(&arena);
}

static void test_lim_metrics(void)
{
	TeX_Layout L = { 0 };
	TexArena arena;
	arena_init(&arena);
	const char* s = "\\lim_{n\\to\\infty}";
	Node* root = tex_parse_math(s, (int)strlen(s), &arena, &L);
	Node* lm = first_child(root);
	expect(lm && lm->type == N_FUNC_LIM, "parsed lim");
	tex_measure_node(lm, FONTROLE_MAIN);
	Node* lim = lm->data.func_lim.limit;
	expect(lm->w >= (lim ? lim->w : 0), "lim width >= under-limit width");
	expect(lm->desc > 0, "lim desc positive due to under-limit");
	arena_free_all(&arena);
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
