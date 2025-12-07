#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tex/tex.h"
#include "tex/tex_internal.h"

// Optional: verbose error callback for debugging
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

static int g_fail = 0;
static void expect(int cond, const char* msg)
{
	if (!cond)
	{
		fprintf(stderr, "[FAIL] %s\n", msg);
		g_fail++;
	}
}

static int count_lines(TeX_Layout* L)
{
	int n = 0;
	TeX_Line* ln = L->lines;
	while (ln)
	{
		++n;
		ln = ln->next;
	}
	return n;
}

static int line_child_count(TeX_Line* ln) { return ln ? ln->child_count : 0; }

static void test_simple_single_line(void)
{
	char buf[] = "a b c";
	TeX_Config cfg = {
		.color_fg = 1, .color_bg = 255, .font_pack = "TeXFonts", .error_callback = test_error_cb, .error_userdata = NULL
	};
	TeX_Layout* L = tex_format(buf, 100, &cfg);
	expect(L != NULL, "tex_format returned layout");
	expect(count_lines(L) == 1, "single line for short text");
	expect(L->total_height > 0, "non-zero total height");
	// With coalescing, adjacent text/space nodes merge into one run
	expect(L->lines && line_child_count(L->lines) == 1, "coalesced text run per line");
	tex_free(L);
}

static void test_display_math_own_line(void)
{
	char buf[] = "x $$y^2$$ z";
	TeX_Config cfg = {
		.color_fg = 1, .color_bg = 255, .font_pack = "TeXFonts", .error_callback = test_error_cb, .error_userdata = NULL
	};
	TeX_Layout* L = tex_format(buf, 50, &cfg);
	expect(L != NULL, "tex_format returned layout (display)");
	expect(count_lines(L) == 3, "display math isolates into its own line with neighbors on separate lines");
	// First line: 'x'
	TeX_Line* l1 = L->lines;
	TeX_Line* l2 = l1 ? l1->next : NULL;
	TeX_Line* l3 = l2 ? l2->next : NULL;
	expect(l1 && line_child_count(l1) == 1, "first line 1 child");
	expect(l2 && line_child_count(l2) == 1, "display line has 1 child (math block)");
	expect(l3 && line_child_count(l3) == 1, "third line 1 child");
	tex_free(L);
}

static void test_newline_break(void)
{
	char buf[] = "a b\nc";
	TeX_Config cfg = {
		.color_fg = 1, .color_bg = 255, .font_pack = "TeXFonts", .error_callback = test_error_cb, .error_userdata = NULL
	};
	TeX_Layout* L = tex_format(buf, 100, &cfg);
	expect(L != NULL, "tex_format returned layout (newline)");
	expect(count_lines(L) == 2, "newline splits lines");
	tex_free(L);
}

static void expect_line_text_equals(TeX_Line* ln, const char* s)
{
	expect(ln != NULL, "line not null");
	expect(line_child_count(ln) == 1, "line has exactly one child after coalescing");
	Node* n = ln->first;
	expect(n != NULL, "first node exists");
	expect(n->type == N_TEXT, "first node is N_TEXT");
	int want = (int)strlen(s);
	expect(n->data.text.len == want, "text length matches");
	expect(strncmp(n->data.text.ptr, s, (size_t)want) == 0, "text content matches");
}

static void test_coalesce_basic_runs(void)
{
	char buf[] = "a b   c"; // multiple spaces collapse to single
	TeX_Config cfg = {
		.color_fg = 1, .color_bg = 255, .font_pack = "TeXFonts", .error_callback = test_error_cb, .error_userdata = NULL
	};
	TeX_Layout* L = tex_format(buf, 100, &cfg);
	expect(L != NULL, "tex_format returned layout");
	expect(count_lines(L) == 1, "single line");
	expect_line_text_equals(L->lines, "a b c");
	tex_free(L);
}

static void test_coalesce_leading_trailing_spaces(void)
{
	char buf[] = "   a    ";
	TeX_Config cfg = {
		.color_fg = 1, .color_bg = 255, .font_pack = "TeXFonts", .error_callback = test_error_cb, .error_userdata = NULL
	};
	TeX_Layout* L = tex_format(buf, 100, &cfg);
	expect(L != NULL, "tex_format returned layout");
	expect(count_lines(L) == 1, "single line");
	expect_line_text_equals(L->lines, "a");
	tex_free(L);
}

static void test_coalesce_multiline_runs(void)
{
	char buf[] = "a  b\nc    d";
	TeX_Config cfg = {
		.color_fg = 1, .color_bg = 255, .font_pack = "TeXFonts", .error_callback = test_error_cb, .error_userdata = NULL
	};
	TeX_Layout* L = tex_format(buf, 100, &cfg);
	expect(L != NULL, "tex_format returned layout");
	expect(count_lines(L) == 2, "two lines");
	TeX_Line* l1 = L->lines;
	TeX_Line* l2 = l1 ? l1->next : NULL;
	expect_line_text_equals(l1, "a b");
	expect_line_text_equals(l2, "c d");
	tex_free(L);
}

static void test_coalesce_around_inline_math(void)
{
	char buf[] = "a $x^2$ b   c";
	TeX_Config cfg = {
		.color_fg = 1, .color_bg = 255, .font_pack = "TeXFonts", .error_callback = test_error_cb, .error_userdata = NULL
	};
	TeX_Layout* L = tex_format(buf, 200, &cfg);
	expect(L != NULL, "tex_format returned layout");
	expect(count_lines(L) == 1, "single line");
	TeX_Line* ln = L->lines;
	expect(ln != NULL, "line exists");
	expect(line_child_count(ln) == 3, "text | math | text");
	Node* a = ln->first;
	Node* m = a ? a->next : NULL;
	Node* b = m ? m->next : NULL;
	expect(a && m && b, "three nodes present");
	expect(a->type == N_TEXT, "left node is text");
	expect(m->type == N_MATH, "middle node is math");
	expect(b->type == N_TEXT, "right node is text");
	// left text should include a trailing space inserted pre-math; right starts with space then words
	expect(a->data.text.len == 2 && strncmp(a->data.text.ptr, "a ", 2) == 0, "left text 'a '");
	expect(b->data.text.len == 4 && strncmp(b->data.text.ptr, " b c", 4) == 0, "right text ' b c'");
	tex_free(L);
}

static void test_zero_copy_single_spaces_ptr(void)
{
	char buf[] = "a b c";
	TeX_Config cfg = {
		.color_fg = 1, .color_bg = 255, .font_pack = "TeXFonts", .error_callback = test_error_cb, .error_userdata = NULL
	};
	TeX_Layout* L = tex_format(buf, 100, &cfg);
	expect(L != NULL, "format");
	expect(count_lines(L) == 1, "one line");
	TeX_Line* ln = L->lines;
	expect(line_child_count(ln) == 1, "one node");
	Node* n = ln->first;
	expect(n->type == N_TEXT, "node is text");
	// Expect zero-copy: pointer should point into original buffer
	const char* p = n->data.text.ptr;
	expect(p == buf, "coalesced ptr equals input start");
	expect(n->data.text.len == (int)strlen("a b c"), "len matches");
	tex_free(L);
}

static void test_copy_when_multi_space_ptr(void)
{
	char buf[] = "a  b"; // double space collapsed -> not contiguous
	TeX_Config cfg = {
		.color_fg = 1, .color_bg = 255, .font_pack = "TeXFonts", .error_callback = test_error_cb, .error_userdata = NULL
	};
	TeX_Layout* L = tex_format(buf, 100, &cfg);
	expect(L != NULL, "format");
	expect(count_lines(L) == 1, "one line");
	Node* n = L->lines->first;
	expect(n->type == N_TEXT, "node is text");
	const char* p = n->data.text.ptr;
	// Pointer should not lie inside the original stack buffer range when copy happened
	if (p >= buf && p < buf + sizeof(buf))
	{
		expect(0, "expected copy; pointer should not be inside input buffer");
	}
	tex_free(L);
}

static void test_long_word_overflow(void)
{
	// Very long word should overflow rather than split
	char buf[] = "abcdefgh"; // 8 chars -> width 48
	TeX_Config cfg = { .color_fg = 1, .color_bg = 255, .font_pack = "TeXFonts" };
	TeX_Layout* L = tex_format(buf, 10, &cfg);
	expect(L != NULL, "format long word");
	expect(count_lines(L) == 1, "single long word stays on one line");
	expect(L->lines && line_child_count(L->lines) == 1, "exactly one node in line");
	tex_free(L);
}

int main(void)
{
	test_simple_single_line();
	test_display_math_own_line();
	test_newline_break();
	test_coalesce_basic_runs();
	test_coalesce_leading_trailing_spaces();
	test_coalesce_multiline_runs();
	test_coalesce_around_inline_math();
	test_zero_copy_single_spaces_ptr();
	test_copy_when_multi_space_ptr();
	// test_wrap_text_narrow();
	// test_wrap_math_atomic_narrow();
	test_long_word_overflow();
	if (g_fail == 0)
	{
		printf("test_layout: PASS\n");
		return 0;
	}
	printf("test_layout: FAIL (%d)\n", g_fail);
	return 1;
}
