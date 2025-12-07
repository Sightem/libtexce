#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tex/tex_internal.h"
#include "tex/tex_parse.h"

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
	arena_init(&L.arena);
	char buf[] = "\\frac{a}{b}";
	Node* root = tex_parse_math(buf, (int)strlen(buf), &L);
	assert_true_int(root && root->type == N_MATH, "root is N_MATH");
	Node* f = root->child;
	assert_true_int(f && f->type == N_FRAC, "frac node present");
	assert_true_int(f->data.frac.num && f->data.frac.den, "frac has num/den");
	Node* num = f->data.frac.num;
	Node* den = f->data.frac.den;
	assert_true_int(num->type == N_MATH && num->child, "num is group math");
	assert_true_int(den->type == N_MATH && den->child, "den is group math");
	assert_true_int(num->child->type == N_GLYPH && num->child->data.glyph == (unsigned char)'a', "num child 'a'");
	assert_true_int(den->child->type == N_GLYPH && den->child->data.glyph == (unsigned char)'b', "den child 'b'");
}

static void test_scripts(void)
{
	TeX_Layout L = { 0 };
	arena_init(&L.arena);
	char buf1[] = "x^2";
	Node* r1 = tex_parse_math(buf1, (int)strlen(buf1), &L);
	Node* n1 = r1->child;
	assert_true_int(n1 && n1->type == N_SCRIPT, "x^2 -> N_SCRIPT");
	assert_true_int(n1->data.script.base && n1->data.script.sup, "sup set");
	assert_true_int(n1->data.script.base->data.glyph == (unsigned char)'x', "base 'x'");
	assert_true_int(n1->data.script.sup->type == N_MATH || n1->data.script.sup->type == N_GLYPH, "sup parsed");

	char buf2[] = "x_1^2";
	Node* r2 = tex_parse_math(buf2, (int)strlen(buf2), &L);
	Node* n2 = r2->child;
	assert_true_int(n2 && n2->type == N_SCRIPT, "x_1^2 -> N_SCRIPT");
	assert_true_int(n2->data.script.sub && n2->data.script.sup, "sub and sup set");
}

static void test_overlays(void)
{
	TeX_Layout L = { 0 };
	arena_init(&L.arena);
	char buf1[] = "\\vec{x}";
	Node* r1 = tex_parse_math(buf1, (int)strlen(buf1), &L);
	Node* n1 = r1->child;
	assert_true_int(n1 && n1->type == N_OVERLAY, "vec overlay");
	assert_true_int(n1->data.overlay.base && n1->data.overlay.base->type == N_MATH, "overlay base is group");

	char buf2[] = "\\hat{ab}";
	Node* r2 = tex_parse_math(buf2, (int)strlen(buf2), &L);
	Node* n2 = r2->child;
	assert_true_int(n2 && n2->type == N_OVERLAY, "hat overlay");
	assert_true_int(n2->data.overlay.base && n2->data.overlay.base->child && n2->data.overlay.base->child->next,
	                "overlay base has 2 children");
}

static void test_lim_and_bigops(void)
{
	TeX_Layout L = { 0 };
	arena_init(&L.arena);
	char buf1[] = "\\lim_{x\\to\\infty}";
	Node* r1 = tex_parse_math(buf1, (int)strlen(buf1), &L);
	Node* n1 = r1->child;
	assert_true_int(n1 && n1->type == N_FUNC_LIM, "lim parsed as N_FUNC_LIM");
	assert_true_int(n1->data.func_lim.limit != NULL, "lim has under limit");

	char buf2[] = "\\sum_{i=0}^n";
	Node* r2 = tex_parse_math(buf2, (int)strlen(buf2), &L);
	Node* n2 = r2->child;
	assert_true_int(n2 && n2->type == N_SCRIPT, "sum scripts -> N_SCRIPT");
	assert_true_int(n2->data.script.base && n2->data.script.sub && n2->data.script.sup, "sum with sub and sup");
}

int main(void)
{
	test_frac();
	test_scripts();
	test_overlays();
	test_lim_and_bigops();
	if (g_fail == 0)
	{
		printf("test_parse: PASS\n");
		return 0;
	}
	printf("test_parse: FAIL (%d)\n", g_fail);
	return 1;
}
