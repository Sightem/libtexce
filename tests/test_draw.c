#include <stdio.h>
#include <string.h>

#include "include/texfont.h"
#include "tex/tex.h"
#include "tex/tex_internal.h"

// Forward declaration for error callback used in configs
static void test_error_cb(void* ud, int level, const char* msg, const char* file, int line);

static int g_fail = 0;
static TeX_Renderer* g_renderer = NULL;

static void expect(int cond, const char* msg)
{
	if (!cond)
	{
		fprintf(stderr, "[FAIL] %s\n", msg);
		g_fail++;
	}
}

static int count_type(TexDrawOpType t)
{
	TexDrawOp ops[1024];
	int n = tex_draw_log_get(ops, 1024);
	int c = 0;
	for (int i = 0; i < n; ++i)
		if (ops[i].type == t)
			++c;
	return c;
}

static int count_glyph(int glyph)
{
	TexDrawOp ops[1024];
	int n = tex_draw_log_get(ops, 1024);
	int c = 0;
	for (int i = 0; i < n; ++i)
		if (ops[i].type == DOP_GLYPH && ops[i].glyph == glyph)
			++c;
	return c;
}

static void test_frac_draws_rule(void)
{
	char buf[] = "$\\frac{a}{bb}$";
	TeX_Config cfg = {
		.color_fg = 1, .color_bg = 255, .font_pack = "TeXFonts", .error_callback = test_error_cb, .error_userdata = NULL
	};
	TeX_Layout* L = tex_format(buf, 100, &cfg);
	expect(L != NULL, "format returns layout");
	tex_draw_log_reset();
	tex_draw(g_renderer, L, 0, 0, 0);
	expect(count_type(DOP_RULE) >= 1, "fraction draws a rule");
	tex_free(L);
}

static void test_overlay_bar_draws_line(void)
{
	char buf[] = "$\\bar{x}$";
	TeX_Config cfg = {
		.color_fg = 1, .color_bg = 255, .font_pack = "TeXFonts", .error_callback = test_error_cb, .error_userdata = NULL
	};
	TeX_Layout* L = tex_format(buf, 100, &cfg);
	tex_draw_log_reset();
	tex_draw(g_renderer, L, 0, 0, 0);
	expect(count_type(DOP_LINE) >= 1, "bar accent draws a line");
	tex_free(L);
}

static void test_sqrt_head_and_bar(void)
{
	char buf[] = "$\\sqrt{xy}$";
	TeX_Config cfg = {
		.color_fg = 1, .color_bg = 255, .font_pack = "TeXFonts", .error_callback = test_error_cb, .error_userdata = NULL
	};
	TeX_Layout* L = tex_format(buf, 100, &cfg);
	tex_draw_log_reset();
	tex_draw(g_renderer, L, 0, 0, 0);
	expect(count_glyph((unsigned char)TEXFONT_SQRT_HEAD_CHAR) >= 1, "sqrt draws radical head glyph");
	expect(count_type(DOP_LINE) >= 1, "sqrt draws overbar line");
	tex_free(L);
}

static void test_viewport_culling(void)
{
	char buf[] = "line1\nline2\nline3\nline4\nline5\nline6\nline7\nline8\nline9\n";
	TeX_Config cfg = {
		.color_fg = 1, .color_bg = 255, .font_pack = "TeXFonts", .error_callback = test_error_cb, .error_userdata = NULL
	};
	TeX_Layout* L = tex_format(buf, 100, &cfg);
	expect(L != NULL, "format returns layout for culling test");
	int total = tex_get_total_height(L);
	tex_draw_log_reset();
	// Scroll past the end: nothing should draw
	tex_draw(g_renderer, L, 0, 0, total + 10);
	expect(tex_draw_log_count() == 0, "culling skips off-screen lines");
	tex_free(L);
}

int main(void)
{
	g_renderer = tex_renderer_create();
	if (!g_renderer)
	{
		fprintf(stderr, "Failed to create renderer\n");
		return 1;
	}

	test_frac_draws_rule();
	test_overlay_bar_draws_line();
	test_sqrt_head_and_bar();
	test_viewport_culling();

	tex_renderer_destroy(g_renderer);

	if (g_fail == 0)
	{
		printf("test_draw: PASS\n");
		return 0;
	}
	printf("test_draw: FAIL (%d)\n", g_fail);
	return 1;
}
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static void test_error_cb(void* ud, int level, const char* msg, const char* file, int line)
{
	(void)ud;
	const char* prefix = (level == 2) ? "ERROR" : (level == 1) ? "WARN" : "INFO";
	fprintf(stderr, "[%s] %s", prefix, msg ? msg : "(null)");
#if TEX_DEBUG
	if (file && line > 0)
	{
		fprintf(stderr, " at %s:%d", file, line);
	}
#else
	(void)file;
	(void)line;
#endif
	fprintf(stderr, "\n");
}
