#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tex/tex_arena.h"
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
	TexArena arena;
	arena_init(&arena);
	char buf[] = "\\frac{a}{b}";
	Node* root = tex_parse_math(buf, (int)strlen(buf), &arena, &L);
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
	arena_free_all(&arena);
}

static void test_scripts(void)
{
	TeX_Layout L = { 0 };
	TexArena arena;
	arena_init(&arena);
	char buf1[] = "x^2";
	Node* r1 = tex_parse_math(buf1, (int)strlen(buf1), &arena, &L);
	Node* n1 = r1->child;
	assert_true_int(n1 && n1->type == N_SCRIPT, "x^2 -> N_SCRIPT");
	assert_true_int(n1->data.script.base && n1->data.script.sup, "sup set");
	assert_true_int(n1->data.script.base->data.glyph == (unsigned char)'x', "base 'x'");
	assert_true_int(n1->data.script.sup->type == N_MATH || n1->data.script.sup->type == N_GLYPH, "sup parsed");

	char buf2[] = "x_1^2";
	Node* r2 = tex_parse_math(buf2, (int)strlen(buf2), &arena, &L);
	Node* n2 = r2->child;
	assert_true_int(n2 && n2->type == N_SCRIPT, "x_1^2 -> N_SCRIPT");
	assert_true_int(n2->data.script.sub && n2->data.script.sup, "sub and sup set");
	arena_free_all(&arena);
}

static void test_overlays(void)
{
	TeX_Layout L = { 0 };
	TexArena arena;
	arena_init(&arena);
	char buf1[] = "\\vec{x}";
	Node* r1 = tex_parse_math(buf1, (int)strlen(buf1), &arena, &L);
	Node* n1 = r1->child;
	assert_true_int(n1 && n1->type == N_OVERLAY, "vec overlay");
	assert_true_int(n1->data.overlay.base && n1->data.overlay.base->type == N_MATH, "overlay base is group");

	char buf2[] = "\\hat{ab}";
	Node* r2 = tex_parse_math(buf2, (int)strlen(buf2), &arena, &L);
	Node* n2 = r2->child;
	assert_true_int(n2 && n2->type == N_OVERLAY, "hat overlay");
	// Parser coalesces adjacent characters into a single N_TEXT node
	assert_true_int(n2->data.overlay.base && n2->data.overlay.base->child, "overlay base has child");
	assert_true_int(n2->data.overlay.base->child->type == N_TEXT, "overlay base child is N_TEXT");
	assert_true_int(n2->data.overlay.base->child->data.text.len == 2, "overlay base text has len 2");
	arena_free_all(&arena);
}

static void test_lim_and_bigops(void)
{
	TeX_Layout L = { 0 };
	TexArena arena;
	arena_init(&arena);
	char buf1[] = "\\lim_{x\\to\\infty}";
	Node* r1 = tex_parse_math(buf1, (int)strlen(buf1), &arena, &L);
	Node* n1 = r1->child;
	assert_true_int(n1 && n1->type == N_FUNC_LIM, "lim parsed as N_FUNC_LIM");
	assert_true_int(n1->data.func_lim.limit != NULL, "lim has under limit");

	char buf2[] = "\\sum_{i=0}^n";
	Node* r2 = tex_parse_math(buf2, (int)strlen(buf2), &arena, &L);
	Node* n2 = r2->child;
	assert_true_int(n2 && n2->type == N_SCRIPT, "sum scripts -> N_SCRIPT");
	assert_true_int(n2->data.script.base && n2->data.script.sub && n2->data.script.sup, "sum with sub and sup");
	arena_free_all(&arena);
}

static void test_auto_delim(void)
{
	TeX_Layout L = { 0 };
	TexArena arena;
	arena_init(&arena);

	// Test basic \left( ... \right)
	char buf1[] = "\\left(x\\right)";
	Node* r1 = tex_parse_math(buf1, (int)strlen(buf1), &arena, &L);
	assert_true_int(r1 && r1->type == N_MATH, "left-right root is N_MATH");
	Node* n1 = r1->child;
	assert_true_int(n1 && n1->type == N_AUTO_DELIM, "left-right parses to N_AUTO_DELIM");
	assert_true_int(n1->data.auto_delim.left_type == DELIM_PAREN, "left type is DELIM_PAREN");
	assert_true_int(n1->data.auto_delim.right_type == DELIM_PAREN, "right type is DELIM_PAREN");
	assert_true_int(n1->data.auto_delim.content != NULL, "auto_delim has content");

	// Test \left. ... \right| (mixed delimiters)
	char buf2[] = "\\left.x\\right|";
	Node* r2 = tex_parse_math(buf2, (int)strlen(buf2), &arena, &L);
	Node* n2 = r2->child;
	assert_true_int(n2 && n2->type == N_AUTO_DELIM, "dot-vert parses to N_AUTO_DELIM");
	assert_true_int(n2->data.auto_delim.left_type == DELIM_NONE, "left type is DELIM_NONE for dot");
	assert_true_int(n2->data.auto_delim.right_type == DELIM_VERT, "right type is DELIM_VERT");

	// Test \left[ ... \right]
	char buf3[] = "\\left[y\\right]";
	Node* r3 = tex_parse_math(buf3, (int)strlen(buf3), &arena, &L);
	Node* n3 = r3->child;
	assert_true_int(n3 && n3->type == N_AUTO_DELIM, "bracket parses to N_AUTO_DELIM");
	assert_true_int(n3->data.auto_delim.left_type == DELIM_BRACKET, "left type is DELIM_BRACKET");
	assert_true_int(n3->data.auto_delim.right_type == DELIM_BRACKET, "right type is DELIM_BRACKET");

	// Test \left\{ ... \right\} (curly braces)
	char buf4[] = "\\left\\{x\\right\\}";
	Node* r4 = tex_parse_math(buf4, (int)strlen(buf4), &arena, &L);
	assert_true_int(r4 && r4->type == N_MATH, "brace left-right root is N_MATH");
	Node* n4 = r4->child;
	assert_true_int(n4 && n4->type == N_AUTO_DELIM, "brace parses to N_AUTO_DELIM");
	assert_true_int(n4->data.auto_delim.left_type == DELIM_BRACE, "left type is DELIM_BRACE");
	assert_true_int(n4->data.auto_delim.right_type == DELIM_BRACE, "right type is DELIM_BRACE");

	arena_free_all(&arena);
}

int main(void)
{
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
