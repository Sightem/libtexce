// test_layout.c - Temporarily stubbed pending windowed rendering integration
//
// The original tests accessed L->lines directly, which is no longer possible
// since TeX_Layout no longer stores lines (they are built on-demand by TeX_Renderer).
//
// TODO: Update tests to use a renderer to trigger rehydration, then inspect lines.

#include <stdio.h>
#include "tex/tex.h"

static int g_fail = 0;

static void test_format_basic(void)
{
	char buf[] = "Hello World";
	TeX_Config cfg = { .color_fg = 1, .color_bg = 255, .font_pack = "TeXFonts" };
	TeX_Layout* L = tex_format(buf, 100, &cfg);
	if (!L)
	{
		fprintf(stderr, "[FAIL] tex_format returned NULL\n");
		g_fail++;
		return;
	}
	if (tex_get_total_height(L) <= 0)
	{
		fprintf(stderr, "[FAIL] total_height should be > 0\n");
		g_fail++;
	}
	tex_free(L);
}

static void test_format_with_math(void)
{
	char buf[] = "The formula $x^2$ is here.";
	TeX_Config cfg = { .color_fg = 1, .color_bg = 255, .font_pack = "TeXFonts" };
	TeX_Layout* L = tex_format(buf, 200, &cfg);
	if (!L)
	{
		fprintf(stderr, "[FAIL] tex_format with math returned NULL\n");
		g_fail++;
		return;
	}
	if (tex_get_total_height(L) <= 0)
	{
		fprintf(stderr, "[FAIL] total_height with math should be > 0\n");
		g_fail++;
	}
	tex_free(L);
}

static void test_format_multiline(void)
{
	char buf[] = "Line 1\nLine 2\nLine 3";
	TeX_Config cfg = { .color_fg = 1, .color_bg = 255, .font_pack = "TeXFonts" };
	TeX_Layout* L = tex_format(buf, 100, &cfg);
	if (!L)
	{
		fprintf(stderr, "[FAIL] tex_format multiline returned NULL\n");
		g_fail++;
		return;
	}
	// Height should accommodate 3 lines
	int h = tex_get_total_height(L);
	if (h <= 5) // At least some height for 3 lines
	{
		fprintf(stderr, "[FAIL] multiline height too small: %d\n", h);
		g_fail++;
	}
	tex_free(L);
}

int main(void)
{
	test_format_basic();
	test_format_with_math();
	test_format_multiline();
	if (g_fail == 0)
	{
		printf("test_layout: PASS\n");
		return 0;
	}
	printf("test_layout: FAIL (%d)\n", g_fail);
	return 1;
}
