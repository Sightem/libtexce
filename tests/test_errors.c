#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tex/tex.h"
#include "tex/tex_internal.h"

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

// Optional verbose error callback
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

static void test_invalid_inputs(void)
{
	TeX_Config cfg = {
		.color_fg = 1, .color_bg = 255, .font_pack = "TeXFonts", .error_callback = test_error_cb, .error_userdata = NULL
	};
	expect(tex_format(NULL, 80, &cfg) == NULL, "NULL input returns NULL layout");
	char s[] = "x";
	expect(tex_format(s, 0, &cfg) == NULL, "non-positive width returns NULL layout");
	expect(tex_format(s, 80, NULL) == NULL, "NULL config returns NULL layout");
}

static void test_malformed_math_draws(void)
{
	TeX_Config cfg = {
		.color_fg = 1, .color_bg = 255, .font_pack = "TeXFonts", .error_callback = test_error_cb, .error_userdata = NULL
	};
	// Unbalanced braces inside math; should not crash
	char buf1[] = "$\\frac{a}{b$ trailing";
	TeX_Layout* L1 = tex_format(buf1, 120, &cfg);
	expect(L1 != NULL, "layout returned for malformed math");
	tex_draw_log_reset();
	tex_draw(g_renderer, L1, 0, 0, 0);
	expect(tex_draw_log_count() >= 0, "draw did not crash on malformed input");
	tex_free(L1);
}

static void test_deep_recursion_guard(void)
{
	TeX_Config cfg = {
		.color_fg = 1, .color_bg = 255, .font_pack = "TeXFonts", .error_callback = test_error_cb, .error_userdata = NULL
	};
	// Build nested groups beyond TEX_PARSE_MAX_DEPTH
	enum
	{
		DEPTH = TEX_PARSE_MAX_DEPTH + 10
	};
	char buf[2048];
	int pos = 0;
	buf[pos++] = '$';
	for (int i = 0; i < DEPTH && pos < (int)sizeof(buf) - 2; ++i)
		buf[pos++] = '{';
	buf[pos++] = 'x';
	for (int i = 0; i < DEPTH && pos < (int)sizeof(buf) - 2; ++i)
		buf[pos++] = '}';
	buf[pos++] = '$';
	buf[pos] = '\0';
	TeX_Layout* L = tex_format(buf, 160, &cfg);
	expect(L != NULL, "layout returned for deep nesting");
	tex_draw_log_reset();
	tex_draw(g_renderer, L, 0, 0, 0);
	expect(tex_draw_log_count() >= 0, "draw did not crash on deep nesting");
	tex_free(L);
}

static void test_height_clamp(void)
{
	TeX_Config cfg = {
		.color_fg = 1, .color_bg = 255, .font_pack = "TeXFonts", .error_callback = test_error_cb, .error_userdata = NULL
	};
	// Create many lines to exceed maximum total height
	char buf[8192];
	int pos = 0;
	for (int i = 0; i < 3000 && pos < (int)sizeof(buf) - 2; ++i)
	{
		buf[pos++] = 'x';
		buf[pos++] = '\n';
	}
	buf[pos] = '\0';
	TeX_Layout* L = tex_format(buf, 80, &cfg);
	expect(L != NULL, "layout returned for tall doc");
	int th = tex_get_total_height(L);
	expect(th <= TEX_MAX_TOTAL_HEIGHT, "total height clamped to maximum");
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

	test_invalid_inputs();
	test_malformed_math_draws();
	test_deep_recursion_guard();
	test_height_clamp();

	tex_renderer_destroy(g_renderer);

	if (g_fail == 0)
	{
		printf("test_errors: PASS\n");
		return 0;
	}
	printf("test_errors: FAIL (%d)\n", g_fail);
	return 1;
}
