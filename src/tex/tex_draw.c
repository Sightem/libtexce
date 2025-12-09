#include <stdlib.h>
#include <string.h>

#include "tex.h"
#include "tex_internal.h"
#include "tex_measure.h"
#include "tex_metrics.h"
#include "tex_parse.h"
#include "tex_renderer.h"
#include "tex_token.h"
#include "tex_util.h"
#include "texfont.h"

#ifdef TEX_DIRECT_RENDER

#ifdef TEX_USE_FONTLIB
#include <fontlibc.h>
#endif
#include <graphx.h>
#if defined(__TICE__)
#include <debug.h>
#endif

static fontlib_font_t* g_draw_font_main = NULL;
static fontlib_font_t* g_draw_font_script = NULL;
static FontRole g_draw_current_role = (FontRole)-1;

void tex_draw_set_fonts(fontlib_font_t* main, fontlib_font_t* script)
{
	g_draw_font_main = main;
	g_draw_font_script = script;
	g_draw_current_role = (FontRole)-1;
}

static inline void ensure_font(FontRole role)
{
	if (role != g_draw_current_role)
	{
		fontlib_SetFont(role ? g_draw_font_script : g_draw_font_main, (fontlib_load_options_t)0);
		g_draw_current_role = (FontRole)role;
	}
}

// -------------------------
// drawing primitives
// -------------------------
static void rec_text(int x, int y_top, const char* s, int len, FontRole role)
{
	int asc = tex_metrics_asc(role);
	int desc = tex_metrics_desc(role);
	int h = asc + desc;

	if (y_top < 0 || (y_top + h) > TEX_VIEWPORT_H)
	{
		return;
	}
	ensure_font(role);
	fontlib_SetCursorPosition((uint24_t)x, (uint8_t)y_top);
	if (s && len > 0)
		fontlib_DrawStringL(s, (size_t)len);
}

static void rec_glyph(int x, int y_top, int glyph, FontRole role)
{
	int asc = tex_metrics_asc(role);
	int desc = tex_metrics_desc(role);
	int h = asc + desc;

	if (y_top < 0 || (y_top + h) > TEX_VIEWPORT_H)
	{
		return;
	}
	ensure_font(role);
	fontlib_SetCursorPosition((uint24_t)x, (uint8_t)y_top);
	fontlib_DrawGlyph((uint8_t)glyph);
}

static void rec_rule(int x, int y, int w)
{
	if (y < 0 || y >= TEX_VIEWPORT_H)
	{
		return;
	}
	gfx_HorizLine(x, y, w);
}

static void rec_line(int x1, int y1, int x2, int y2)
{

	if ((y1 < 0 && y2 < 0) || (y1 >= TEX_VIEWPORT_H && y2 >= TEX_VIEWPORT_H))
	{
		return;
	}
	gfx_Line(x1, y1, x2, y2);
}

static void rec_dot(int cx, int cy)
{
	if (cy < 0 || cy >= TEX_VIEWPORT_H)
	{
		return;
	}
	gfx_FillCircle(cx, cy, 1);
}

static void rec_ellipse(int cx, int cy, int rx, int ry)
{
	if ((cy + ry) < 0 || (cy - ry) >= TEX_VIEWPORT_H)
	{
		return;
	}
	if (rx < 0 || ry < 0)
		return;
	gfx_Ellipse(cx, cy, (uint24_t)rx, (uint24_t)ry);
}


static void rec_draw_paren(int x, int y_center, int w, int h, int is_left)
{
	if (h <= 0 || w <= 0)
		return;

	int ry = h / 2; // vertical radius (half height)
	int rx = w - 1; // horizontal radius

	static const int cos_table[7] = { 0, 128, 221, 256, 221, 128, 0 };
	static const int sin_table[7] = { -256, -221, -128, 0, 128, 221, 256 };

	int cx, prev_px, prev_py;

	if (is_left)
	{
		cx = x + w - 1;
		prev_px = cx - (rx * cos_table[0]) / 256;
		prev_py = y_center + (ry * sin_table[0]) / 256;
	}
	else
	{
		cx = x; // center at left edge
		prev_px = cx + (rx * cos_table[0]) / 256;
		prev_py = y_center + (ry * sin_table[0]) / 256;
	}

	for (int i = 1; i < 7; i++)
	{
		int cur_px, cur_py;
		cur_py = y_center + (ry * sin_table[i]) / 256;

		if (is_left)
			cur_px = cx - (rx * cos_table[i]) / 256;
		else
			cur_px = cx + (rx * cos_table[i]) / 256;

		rec_line(prev_px, prev_py, cur_px, cur_py);
		prev_px = cur_px;
		prev_py = cur_py;
	}
}

void tex_draw_log_reset(void) {}
int tex_draw_log_count(void) { return 0; }
int tex_draw_log_get(TexDrawOp* out, int max)
{
	(void)out;
	(void)max;
	return 0;
}
int tex_draw_log_get_range(TexDrawOp* out, int start, int count)
{
	(void)out;
	(void)start;
	(void)count;
	return 0;
}

#else // TEX_DIRECT_RENDER

// -------------------------
// Draw op recorder
// -------------------------
#define TEX_DRAW_LOG_CAP 4096

static TexDrawOp g_log_buf[TEX_DRAW_LOG_CAP];
static int g_log_n = 0;
static int g_log_dropped = 0;

static void log_op(const TexDrawOp* op)
{
	if (!op)
		return;
	if (g_log_n < TEX_DRAW_LOG_CAP)
	{
		g_log_buf[g_log_n++] = *op;
	}
	else
	{
		g_log_dropped = 1;
	}
}

void tex_draw_log_reset(void)
{
	g_log_n = 0;
	g_log_dropped = 0;
}

int tex_draw_log_count(void) { return g_log_n; }

int tex_draw_log_get(TexDrawOp* out, int max) { return tex_draw_log_get_range(out, 0, max); }

int tex_draw_log_get_range(TexDrawOp* out, int start_index, int count)
{
	if (start_index < 0 || start_index >= g_log_n)
		return 0;
	int available = g_log_n - start_index;
	int n = (available < count) ? available : count;
	if (out && n > 0)
		memcpy(out, &g_log_buf[start_index], (size_t)n * sizeof(*out));
	return n;
}

// -------------------------
// Drawing primitives
// -------------------------
static void rec_text(int x, int y_top, const char* s, int len, FontRole role)
{
	TexDrawOp op = { DOP_TEXT, x, y_top, 0, 0, 0, 0, 0, s, len, (int)role };
	log_op(&op);
}

static void rec_glyph(int x, int y_top, int glyph, FontRole role)
{
	TexDrawOp op = { DOP_GLYPH, x, y_top, 0, 0, 0, 0, glyph, NULL, 0, (int)role };
	log_op(&op);
}

static void rec_rule(int x, int y, int w)
{
	TexDrawOp op = { DOP_RULE, x, y, x + w, y, w, TEX_RULE_THICKNESS, 0, NULL, 0, 0 };
	log_op(&op);
}

static void rec_line(int x1, int y1, int x2, int y2)
{
	TexDrawOp op = { DOP_LINE, x1, y1, x2, y2, 0, 0, 0, NULL, 0, 0 };
	log_op(&op);
}

static void rec_dot(int cx, int cy)
{
	TexDrawOp op = { DOP_DOT, cx, cy, 0, 0, 0, 0, 0, NULL, 0, 0 };
	log_op(&op);
}

static void rec_ellipse(int cx, int cy, int rx, int ry)
{
	TexDrawOp op = { DOP_ELLIPSE, cx, cy, 0, 0, rx, ry, 0, NULL, 0, 0 };
	log_op(&op);
}

static void rec_draw_paren(int x, int y_center, int w, int h, int is_left)
{
	if (h <= 0 || w <= 0)
		return;

	int ry = h / 2;
	int rx = w - 1;

	static const int cos_table[7] = { 0, 128, 221, 256, 221, 128, 0 };
	static const int sin_table[7] = { -256, -221, -128, 0, 128, 221, 256 };

	int cx, prev_px, prev_py;

	if (is_left)
	{
		cx = x + w - 1;
		prev_px = cx - (rx * cos_table[0]) / 256;
		prev_py = y_center + (ry * sin_table[0]) / 256;
	}
	else
	{
		cx = x;
		prev_px = cx + (rx * cos_table[0]) / 256;
		prev_py = y_center + (ry * sin_table[0]) / 256;
	}

	for (int i = 1; i < 7; i++)
	{
		int cur_px, cur_py;
		cur_py = y_center + (ry * sin_table[i]) / 256;

		if (is_left)
			cur_px = cx - (rx * cos_table[i]) / 256;
		else
			cur_px = cx + (rx * cos_table[i]) / 256;

		rec_line(prev_px, prev_py, cur_px, cur_py);
		prev_px = cur_px;
		prev_py = cur_py;
	}
}

#endif // TEX_DIRECT_RENDER

// -------------------------
// Node draw routines
// -------------------------
static int g_axis_y = 0;
static void draw_node(Node* n, int x, int baseline_y, FontRole role);

static void draw_math_list(Node* head, int x, int baseline_y, FontRole role)
{
	int xc = x;
	for (Node* it = head; it; it = it->next)
	{
		draw_node(it, xc, baseline_y, role);
		xc += it->w;
	}
}


static void draw_script(Node* n, int x, int baseline_y, FontRole role)
{
	Node* base = n->data.script.base;
	Node* sub = n->data.script.sub;
	Node* sup = n->data.script.sup;

	if (base)
		draw_node(base, x, baseline_y, role);

	int x_scripts = x + (base ? base->w : 0) + TEX_SCRIPT_XPAD;
	int is_bigop = base && tex_node_is_big_operator(base);

	// precalculate big operator vertical bounds using the math axis
	int op_top = 0, op_bot = 0;
	if (is_bigop)
	{
		int op_bias = 0;

		if (base->type == N_MULTIOP)
		{
			if (base->data.multiop.op_type == MULTIOP_INT)
				op_bias = TEX_AXIS_BIAS_INTEGRAL;
			// TODO: MULTIOP_OINT would also use INTEGRAL bias
		}
		else if (base->type == N_GLYPH)
		{
			// Single big-op glyphs: check character code
			unsigned char g = (unsigned char)base->data.glyph;
			if (g == (unsigned char)TEXFONT_INTEGRAL_CHAR)
				op_bias = TEX_AXIS_BIAS_INTEGRAL;
			else if (g == (unsigned char)TEXFONT_SUMMATION_CHAR)
				op_bias = TEX_AXIS_BIAS_SUM;
			else if (g == (unsigned char)TEXFONT_PRODUCT_CHAR)
				op_bias = TEX_AXIS_BIAS_PROD;
		}

		int half = (base->asc + base->desc) / 2;
		int axis = g_axis_y + op_bias;
		op_top = axis - half;
		op_bot = axis + half;
	}

	// determine baselines for scripts
	if (sup)
	{
		int sup_baseline;
		if (is_bigop)
		{
			// anchor bottom of superscript to top of operator
			sup_baseline = (op_top + TEX_BIGOP_OVERLAP) - sup->desc;
		}
		else
		{
			int center_y;
			if (base)
				center_y = baseline_y + (base->desc - base->asc) / 2;
			else
				center_y = baseline_y - (tex_metrics_asc(role) / 2);

			// place just above center
			sup_baseline = center_y - sup->desc - 1;
		}
		draw_node(sup, x_scripts, sup_baseline, FONTROLE_SCRIPT);
	}

	if (sub)
	{
		int sub_baseline;
		if (is_bigop)
		{
			sub_baseline = (op_bot - TEX_BIGOP_OVERLAP) + sub->asc;
		}
		else
		{
			int center_y;
			if (base)
				center_y = baseline_y + (base->desc - base->asc) / 2;
			else
				center_y = baseline_y - (tex_metrics_asc(role) / 2);

			sub_baseline = center_y + sub->asc + 1;
		}
		draw_node(sub, x_scripts, sub_baseline, FONTROLE_SCRIPT);
	}
}

static void draw_frac(Node* n, int x, int baseline_y)
{
	Node* num = n->data.frac.num;
	Node* den = n->data.frac.den;

	int axis = tex_metrics_math_axis();
	int rule_y = baseline_y - axis;

	int rule_x = x + TEX_FRAC_OUTER_PAD;
	int rule_w = n->w - (2 * TEX_FRAC_OUTER_PAD);

	rec_rule(rule_x, rule_y, rule_w);

	if (num)
	{
		int nx = x + (n->w - num->w) / 2;
		int num_baseline = rule_y - TEX_FRAC_YPAD - num->desc;
		draw_node(num, nx, num_baseline, FONTROLE_SCRIPT);
	}
	if (den)
	{
		int dx = x + (n->w - den->w) / 2;
		int den_baseline = rule_y + TEX_RULE_THICKNESS + TEX_FRAC_YPAD + den->asc;
		draw_node(den, dx, den_baseline, FONTROLE_SCRIPT);
	}
}

static void draw_sqrt(Node* n, int x, int baseline_y, FontRole role)
{
	Node* rad = n->data.sqrt.rad;
	Node* idx = n->data.sqrt.index;

	int head_w = tex_metrics_glyph_width((unsigned char)TEXFONT_SQRT_HEAD_CHAR, role);

	int idx_w = idx ? idx->w : 0;
	int idx_offset = 0;
	if (idx)
	{
		idx_offset = idx_w + TEX_SQRT_INDEX_KERNING;
		if (idx_offset < 0)
			idx_offset = 0;
	}

	// position of radical head (shifted right to clear the index)
	int head_x = x + idx_offset;
	int head_y_top = baseline_y - tex_metrics_asc(role);
	rec_glyph(head_x, head_y_top, (unsigned char)TEXFONT_SQRT_HEAD_CHAR, role);

	// draw index if present
	if (idx)
	{
		// index baseline is roughly halfway up the radical height to try to "tuck" it in
		int idx_baseline = baseline_y - (tex_metrics_asc(role) / 2);
		int idx_x = x; // left aligned with the bounding box
		draw_node(idx, idx_x, idx_baseline, FONTROLE_SCRIPT);
	}

	// draw bar and radicand TODO: fix this
	int bar_x = head_x + head_w + TEX_SQRT_HEAD_XPAD;
	if (rad)
	{
		int bar_y = baseline_y - rad->asc - TEX_ACCENT_GAP;

		int width = (x + n->w) - bar_x;
		rec_line(bar_x, bar_y, bar_x + width, bar_y);
		draw_node(rad, bar_x, baseline_y, role);
	}
}


static void draw_overlay(Node* n, int x, int baseline_y, FontRole role)
{
	Node* b = n->data.overlay.base;

	// draw the base character first
	if (b)
		draw_node(b, x, baseline_y, role);

	if (!b)
		return;

	// Y position just above the character's ascent + gap
	int top = baseline_y - b->asc - TEX_ACCENT_GAP;

	switch (n->data.overlay.type)
	{
	case ACC_BAR:
		{
			int bar_y = top - 1;

			int pad = (b->w > 2) ? 1 : 0;

			// draw from left+pad to right-pad-1 for fencepost
			rec_line(x + pad, bar_y, x + b->w - 1 - pad, bar_y);
		}
		break;

	case ACC_OVERLINE:
		{
			int line_y = top - 1;
			int pad = (b->w > 2) ? 1 : 0;
			rec_line(x + pad, line_y, x + b->w - 1 - pad, line_y);
		}
		break;

	case ACC_UNDERLINE:
		{
			int line_y = baseline_y + b->desc + TEX_ACCENT_GAP;
			int pad = (b->w > 2) ? 1 : 0;
			rec_line(x + pad, line_y, x + b->w - 1 - pad, line_y);
		}
		break;

	case ACC_DOT:
		{
			int cx = x + b->w / 2;
			// draw dot slightly above the gap
			rec_dot(cx, top - 1);
		}
		break;

	case ACC_HAT:
		{
			int cx = x + b->w / 2;
			int dy = 3; // height of the chevron

			// left leg
			rec_line(cx - dy, top, cx, top - dy);
			// right leg
			rec_line(cx, top - dy, cx + dy, top);
		}
		break;

	case ACC_VEC:
		{
			int len = TEX_MAX(5, b->w);

			int x_end = x + b->w; // right edge of character
			int x_start = x_end - len; // calculated left start point of arrow
			int y = top - 2;

			// draw shaft
			rec_line(x_start, y, x_end, y);

			// dx=3, dy=2 prevents the pixels from clumping into a blob at low res
			rec_line(x_end - 3, y - 2, x_end, y);
			rec_line(x_end - 3, y + 2, x_end, y);
		}
		break;

	case ACC_DDOT:
		{
			int cx = x + b->w / 2;
			// separate dots by 4px total (2px offset each way)
			// rec_dot draws radius 1 (approx 3px diameter)
			int sep = 2;

			// draw left dot
			rec_dot(cx - sep, top - 1);

			// draw right dot
			rec_dot(cx + sep, top - 1);
		}
		break;

	default:
		break;
	}
}

static void draw_hbrace(int x, int y, int w, int is_over)
{
	if (w <= 0)
		return;
	if (w < 6)
	{
		// too small for a visible brace, fallback to short line
		rec_line(x, y + (is_over ? (TEX_BRACE_HEIGHT - 2) : 1), x + w - 1, y + (is_over ? (TEX_BRACE_HEIGHT - 2) : 1));
		return;
	}
	int mid = x + w / 2;
	int arm_h = 2;
	int top_y = y;
	if (is_over)
	{
		// arms down, y is top of brace box
		int base_y = top_y + arm_h;
		rec_line(x, base_y, mid - 2, base_y);
		rec_line(mid - 2, base_y, mid, top_y);
		rec_line(mid, top_y, mid + 2, base_y);
		rec_line(mid + 2, base_y, x + w - 1, base_y);
	}
	else
	{
		// arms up, y is baseline of arms
		int base_y = y;
		rec_line(x, base_y, mid - 2, base_y);
		rec_line(mid - 2, base_y, mid, base_y + arm_h + (TEX_BRACE_HEIGHT - arm_h));
		rec_line(mid, base_y + arm_h + (TEX_BRACE_HEIGHT - arm_h), mid + 2, base_y);
		rec_line(mid + 2, base_y, x + w - 1, base_y);
	}
}

static void draw_spandeco(Node* n, int x, int baseline_y, FontRole role)
{
	Node* content = n->data.spandeco.content;
	Node* label = n->data.spandeco.label;
	if (content)
		draw_node(content, x, baseline_y, role);
	int w = content ? content->w : 0;
	int bh = TEX_BRACE_HEIGHT;
	if (n->data.spandeco.deco_type == DECO_OVERBRACE)
	{
		int brace_y = baseline_y - (content ? content->asc : 0) - TEX_ACCENT_GAP - bh + 1; // y is top of brace box
		draw_hbrace(x, brace_y, w, 1);
		if (label)
		{
			int lx = x + (w - label->w) / 2;
			int label_baseline = brace_y - TEX_ACCENT_GAP - label->desc;
			draw_node(label, lx, label_baseline, FONTROLE_SCRIPT);
		}
	}
	else if (n->data.spandeco.deco_type == DECO_UNDERBRACE)
	{
		int brace_y = baseline_y + (content ? content->desc : 0) + TEX_ACCENT_GAP; // y is baseline of arms
		draw_hbrace(x, brace_y, w, 0);
		if (label)
		{
			int lx = x + (w - label->w) / 2;
			int label_baseline = brace_y + bh + TEX_ACCENT_GAP + label->asc;
			draw_node(label, lx, label_baseline, FONTROLE_SCRIPT);
		}
	}
}

static void draw_multiop(Node* n, int x)
{
	if (!n)
		return;

	uint8_t count = n->data.multiop.count;
	if (count < 1)
		count = 1;

	FontRole effective_role = FONTROLE_MAIN;

	int glyph_w = tex_metrics_glyph_width((unsigned char)TEXFONT_INTEGRAL_CHAR, effective_role);
	int kern = TEX_MULTIOP_KERN;

	int half = (n->asc + n->desc) / 2;
	int bias = TEX_AXIS_BIAS_INTEGRAL;
	int y_top = (g_axis_y + bias) - half;

	int cur_x = x;
	for (uint8_t i = 0; i < count; i++)
	{
		rec_glyph(cur_x, y_top, (int)TEXFONT_INTEGRAL_CHAR, effective_role);
		cur_x += glyph_w + kern;
	}

	// for contour integrals, draw a procedural ellipse centered on the operator group
	if (n->data.multiop.op_type == MULTIOP_OINT)
	{
		// center of the entire multi-operator block
		// use (w - 1) / 2 to bias left for even widths, correcting the 1px right shift
		int cx = x + (n->w - 1) / 2;
		// center Y aligns with the math axis + bias used for the integral glyphs
		int cy = g_axis_y + bias;

		// Rx is half the total width (spanning all integrals)
		int rx = n->w / 2;
		// Ry is half the width of a single integral glyph (heuristic for oval shape)
		int ry = glyph_w / 2;

		rec_ellipse(cx, cy, rx, ry);
	}
}

static void draw_func_lim(Node* n, int x, int baseline_y)
{
	// draw "lim" upright at baseline
	int y_top = baseline_y - tex_metrics_asc(FONTROLE_MAIN);
	rec_text(x, y_top, "lim", 3, FONTROLE_MAIN);
	Node* lim = n->data.func_lim.limit;
	if (lim)
	{
		int lim_text_w = tex_metrics_text_width("lim", FONTROLE_MAIN);
		int lx = x + (lim_text_w - lim->w) / 2;
		int lim_baseline = baseline_y + TEX_FRAC_YPAD + TEX_RULE_THICKNESS + lim->asc;
		draw_node(lim, lx, lim_baseline, FONTROLE_SCRIPT);
	}
}

static void draw_proc_delim(int x, int y_center, int h, DelimType type, int is_left)
{
	if (type == DELIM_NONE)
		return;

	int w = h / TEX_DELIM_WIDTH_FACTOR;
	w = TEX_CLAMP(w, TEX_DELIM_MIN_WIDTH, TEX_DELIM_MAX_WIDTH);
	int top = y_center - (h / 2);
	int bot = y_center + (h / 2);

	switch (type)
	{
	case DELIM_PAREN:
		rec_draw_paren(x, y_center, w, h, is_left);
		break;
	case DELIM_BRACKET:
		if (is_left)
		{
			rec_line(x, top, x, bot);
			rec_line(x, top, x + w / 2, top);
			rec_line(x, bot, x + w / 2, bot);
		}
		else
		{
			rec_line(x + w - 1, top, x + w - 1, bot);
			rec_line(x + w / 2, top, x + w - 1, top);
			rec_line(x + w / 2, bot, x + w - 1, bot);
		}
		break;
	case DELIM_BRACE:
		{
			// constructed of: horizontal line, leg, chevron, leg, horizontal line
			int half_h = h / 2;
			// height of the chevron part (beak) - scale with width but bound by height
			// use w/2 to keep the chevron compact (roughly 90 deg tip) and legs long
			int chev_dy = w / 2;
			if (chev_dy < 3)
				chev_dy = 3;
			if (chev_dy > half_h)
				chev_dy = half_h;

			if (is_left)
			{
				// {
				// spine is to the right
				int spine_x = x + w / 2;

				// top horizontal
				rec_line(x + w - 1, top, spine_x, top);
				// top leg
				rec_line(spine_x, top, spine_x, y_center - chev_dy);
				// chevron upper
				rec_line(spine_x, y_center - chev_dy, x, y_center);
				// chevron lower
				rec_line(x, y_center, spine_x, y_center + chev_dy);
				// bottom leg
				rec_line(spine_x, y_center + chev_dy, spine_x, bot);
				// bottom horizontal
				rec_line(spine_x, bot, x + w - 1, bot);
			}
			else
			{
				// }
				// spine is to the left
				int spine_x = x + (w - 1) / 2;

				// top horizontal
				rec_line(x, top, spine_x, top);
				// top leg
				rec_line(spine_x, top, spine_x, y_center - chev_dy);
				// chevron upper
				rec_line(spine_x, y_center - chev_dy, x + w - 1, y_center);
				// chevron lower
				rec_line(x + w - 1, y_center, spine_x, y_center + chev_dy);
				// bottom leg
				rec_line(spine_x, y_center + chev_dy, spine_x, bot);
				// bottom horizontal
				rec_line(spine_x, bot, x, bot);
			}
		}
		break;
	case DELIM_VERT:
		rec_line(x + w / 2, top, x + w / 2, bot);
		break;
	case DELIM_ANGLE:
		if (is_left)
		{
			rec_line(x + w - 1, top, x, y_center);
			rec_line(x, y_center, x + w - 1, bot);
		}
		else
		{
			rec_line(x, top, x + w - 1, y_center);
			rec_line(x + w - 1, y_center, x, bot);
		}
		break;
	case DELIM_FLOOR:
		if (is_left)
		{
			rec_line(x, top, x, bot);
			rec_line(x, bot, x + w / 2, bot);
		}
		else
		{
			rec_line(x + w - 1, top, x + w - 1, bot);
			rec_line(x + w / 2, bot, x + w - 1, bot);
		}
		break;
	case DELIM_CEIL:
		if (is_left)
		{
			rec_line(x, top, x, bot);
			rec_line(x, top, x + w / 2, top);
		}
		else
		{
			rec_line(x + w - 1, top, x + w - 1, bot);
			rec_line(x + w / 2, top, x + w - 1, top);
		}
		break;
	default:
		break;
	}
}

static void draw_auto_delim(Node* n, int x, int baseline_y, FontRole role)
{
	int h = n->data.auto_delim.delim_h;
	int axis = tex_metrics_math_axis();
	int y_center = baseline_y - axis;

	int delim_w = h / TEX_DELIM_WIDTH_FACTOR;
	delim_w = TEX_CLAMP(delim_w, TEX_DELIM_MIN_WIDTH, TEX_DELIM_MAX_WIDTH);

	int l_w = (n->data.auto_delim.left_type == DELIM_NONE) ? 0 : delim_w;
	int r_w = (n->data.auto_delim.right_type == DELIM_NONE) ? 0 : delim_w;

	if (l_w > 0)
		draw_proc_delim(x, y_center, h, (DelimType)n->data.auto_delim.left_type, 1);

	if (n->data.auto_delim.content)
		draw_math_list(n->data.auto_delim.content, x + l_w, baseline_y, role);

	if (r_w > 0)
	{
		int c_w = 0;
		for (Node* it = n->data.auto_delim.content; it; it = it->next)
			c_w += it->w;
		int rx = x + l_w + c_w;
		draw_proc_delim(rx, y_center, h, (DelimType)n->data.auto_delim.right_type, 0);
	}
}

static void draw_node(Node* n, int x, int baseline_y, FontRole role)
{
	if (!n)
		return;
	switch (n->type)
	{
	case N_TEXT:
		{
			int y_top = baseline_y - n->asc;
			const char* s = n->data.text.ptr ? n->data.text.ptr : "";
			int len = n->data.text.len;
			if (!s || len <= 0)
			{
				s = "";
				len = 0;
			}
			rec_text(x, y_top, s, len, role);
		}
		break;
	case N_GLYPH:
		{
			// big operators (integral, summation, product) always use main font to maintain legibility even in nested
			// contexts like fractions
			FontRole effective_role = role;
			int is_bigop = tex_is_big_operator(n->data.glyph);
			if (is_bigop)
			{
				effective_role = FONTROLE_MAIN;
				int half = (n->asc + n->desc) / 2;
				int bias = 0;
				unsigned char g = (unsigned char)n->data.glyph;
				if (g == (unsigned char)TEXFONT_INTEGRAL_CHAR)
					bias = TEX_AXIS_BIAS_INTEGRAL;
				else if (g == (unsigned char)TEXFONT_SUMMATION_CHAR)
					bias = TEX_AXIS_BIAS_SUM;
				else if (g == (unsigned char)TEXFONT_PRODUCT_CHAR)
					bias = TEX_AXIS_BIAS_PROD;
				int y_top = (g_axis_y + bias) - half;
				rec_glyph(x, y_top, (int)n->data.glyph, effective_role);
			}
			else
			{
				int y_top = baseline_y - n->asc;
				rec_glyph(x, y_top, (int)n->data.glyph, effective_role);
			}
		}
		break;
	case N_SPACE:
		// nop, spacing is applied via measured width
		break;
	case N_MATH:
		draw_math_list(n->child, x, baseline_y, role);
		break;
	case N_SCRIPT:
		draw_script(n, x, baseline_y, role);
		break;
	case N_FRAC:
		draw_frac(n, x, baseline_y);
		break;
	case N_SQRT:
		draw_sqrt(n, x, baseline_y, role);
		break;
	case N_OVERLAY:
		draw_overlay(n, x, baseline_y, role);
		break;
	case N_SPANDECO:
		draw_spandeco(n, x, baseline_y, role);
		break;
	case N_FUNC_LIM:
		draw_func_lim(n, x, baseline_y);
		break;
	case N_MULTIOP:
		draw_multiop(n, x);
		break;
	case N_AUTO_DELIM:
		draw_auto_delim(n, x, baseline_y, role);
		break;
	default:
		break;
	}
}

static int find_checkpoint_index(TeX_Layout* L, int target_y)
{
	if (!L->checkpoints || L->checkpoint_count == 0)
		return -1;

	int lo = 0, hi = L->checkpoint_count;
	while (lo < hi)
	{
		int mid = (lo + hi) / 2;
		if (L->checkpoints[mid].y_pos <= target_y)
			lo = mid + 1;
		else
			hi = mid;
	}

	return (lo > 0) ? (lo - 1) : -1;
}

static void rehydrate_window(TeX_Renderer* r, TeX_Layout* layout, int scroll_y)
{
	int padded_top = scroll_y - TEX_RENDERER_PADDING;
	int padded_bot = scroll_y + TEX_VIEWPORT_H + TEX_RENDERER_PADDING;
	if (padded_top < 0)
		padded_top = 0;
	if (padded_bot > layout->total_height)
		padded_bot = layout->total_height;

	arena_reset(&r->arena);
	r->visible_lines = NULL;
	r->line_count = 0;
	r->line_index = NULL;

	// find starting checkpoint
	int cp_idx = find_checkpoint_index(layout, padded_top);
	const char* src_start;
	int y_start;

	if (cp_idx >= 0 && layout->checkpoints)
	{
		src_start = layout->checkpoints[cp_idx].src_ptr;
		y_start = layout->checkpoints[cp_idx].y_pos;
	}
	else
	{
		src_start = layout->source;
		y_start = 0;
	}
	TeX_Line* head = NULL;
	TeX_Line* tail_line = NULL;
	Node* cur_line_head = NULL;
	Node* cur_line_tail = NULL;
	int16_t x_cursor = 0;
	int16_t line_asc = 0;
	int16_t line_desc = 0;
	int current_y = y_start;
	int pending_space = 0;
	int line_count = 0;

	TeX_Stream stream;
	tex_stream_init(&stream, src_start, -1);

	TeX_Token t;
	while (tex_stream_next(&stream, &t, &r->arena, layout))
	{
		if (current_y >= padded_bot)
			break;

		switch (t.type)
		{
		case T_NEWLINE:
			{
				// for blank lines (no content), set default font height
				int eff_asc = line_asc;
				int eff_desc = line_desc;
				if (!cur_line_head && eff_asc == 0 && eff_desc == 0)
				{
					eff_asc = tex_metrics_asc(FONTROLE_MAIN);
					eff_desc = tex_metrics_desc(FONTROLE_MAIN);
				}

				// finalize current line only if there's content or metrics
				if (cur_line_head || eff_asc > 0 || eff_desc > 0)
				{
					int h = eff_asc + eff_desc + TEX_LINE_LEADING;
					if (h <= 0)
						h = 1;

					TeX_Line* ln = (TeX_Line*)arena_alloc(&r->arena, sizeof(TeX_Line), sizeof(void*));
					if (ln)
					{
						memset(ln, 0, sizeof(TeX_Line));
						ln->first = cur_line_head;
						ln->y = current_y;
						ln->h = h;
						ln->next = NULL;

						if (tail_line)
							tail_line->next = ln;
						else
							head = ln;
						tail_line = ln;
						line_count++;
					}

					current_y += h;
					cur_line_head = NULL;
					cur_line_tail = NULL;
					x_cursor = 0;
					line_asc = 0;
					line_desc = 0;
				}
				pending_space = 0;
			}
			break;

		case T_SPACE:
			pending_space = 1;
			break;

		case T_TEXT:
			{
				int16_t text_w = tex_metrics_text_width_n(t.start, t.len, FONTROLE_MAIN);
				int16_t text_asc = tex_metrics_asc(FONTROLE_MAIN);
				int16_t text_desc = tex_metrics_desc(FONTROLE_MAIN);

				if (pending_space && cur_line_head)
				{
					int16_t space_w = tex_metrics_text_width_n(" ", 1, FONTROLE_MAIN);
					// simple wrap check
					if (x_cursor + space_w + text_w > layout->width && cur_line_head)
					{
						// finalize line before wrap
						int h = line_asc + line_desc + TEX_LINE_LEADING;
						if (h <= 0)
							h = 1;
						TeX_Line* ln = (TeX_Line*)arena_alloc(&r->arena, sizeof(TeX_Line), sizeof(void*));
						if (ln)
						{
							memset(ln, 0, sizeof(TeX_Line));
							ln->first = cur_line_head;
							ln->y = current_y;
							ln->h = h;
							if (tail_line)
								tail_line->next = ln;
							else
								head = ln;
							tail_line = ln;
							line_count++;
						}
						current_y += h;
						cur_line_head = NULL;
						cur_line_tail = NULL;
						x_cursor = 0;
						line_asc = 0;
						line_desc = 0;
					}
					else
					{
						Node* sp = (Node*)arena_alloc(&r->arena, sizeof(Node), sizeof(void*));
						if (sp)
						{
							memset(sp, 0, sizeof(Node));
							sp->type = N_TEXT;
							sp->data.text.ptr = " ";
							sp->data.text.len = 1;
							sp->w = space_w;
							sp->asc = text_asc;
							sp->desc = text_desc;
							sp->x = x_cursor;

							if (cur_line_tail)
								cur_line_tail->next = sp;
							else
								cur_line_head = sp;
							cur_line_tail = sp;
							x_cursor += space_w;
							line_asc = (int16_t)TEX_MAX(line_asc, text_asc);
							line_desc = (int16_t)TEX_MAX(line_desc, text_desc);
						}
					}
				}
				pending_space = 0;

				Node* n = (Node*)arena_alloc(&r->arena, sizeof(Node), sizeof(void*));
				if (n)
				{
					memset(n, 0, sizeof(Node));
					n->type = N_TEXT;
					n->data.text.ptr = t.start;
					n->data.text.len = t.len;
					n->w = text_w;
					n->asc = text_asc;
					n->desc = text_desc;
					n->x = x_cursor;

					if (cur_line_tail)
						cur_line_tail->next = n;
					else
						cur_line_head = n;
					cur_line_tail = n;
					x_cursor += text_w;
					line_asc = (int16_t)TEX_MAX(line_asc, text_asc);
					line_desc = (int16_t)TEX_MAX(line_desc, text_desc);
				}
			}
			break;

		case T_MATH_INLINE:
			{
				Node* n = tex_parse_math(t.start, t.len, &r->arena, layout);
				if (n)
				{
					n->flags &= ~0x01; // clear display flag
					tex_measure_node(n, FONTROLE_MAIN);

					// handle pending space
					if (pending_space && cur_line_head)
					{
						int16_t space_w = tex_metrics_text_width_n(" ", 1, FONTROLE_MAIN);
						int16_t space_asc = tex_metrics_asc(FONTROLE_MAIN);
						int16_t space_desc = tex_metrics_desc(FONTROLE_MAIN);

						if (x_cursor + space_w + n->w > layout->width && cur_line_head)
						{
							int h = line_asc + line_desc + TEX_LINE_LEADING;
							if (h <= 0)
								h = 1;
							TeX_Line* ln = (TeX_Line*)arena_alloc(&r->arena, sizeof(TeX_Line), sizeof(void*));
							if (ln)
							{
								memset(ln, 0, sizeof(TeX_Line));
								ln->first = cur_line_head;
								ln->y = current_y;
								ln->h = h;
								if (tail_line)
									tail_line->next = ln;
								else
									head = ln;
								tail_line = ln;
								line_count++;
							}
							current_y += h;
							cur_line_head = NULL;
							cur_line_tail = NULL;
							x_cursor = 0;
							line_asc = 0;
							line_desc = 0;
						}
						else
						{
							Node* sp = (Node*)arena_alloc(&r->arena, sizeof(Node), sizeof(void*));
							if (sp)
							{
								memset(sp, 0, sizeof(Node));
								sp->type = N_TEXT;
								sp->data.text.ptr = " ";
								sp->data.text.len = 1;
								sp->w = space_w;
								sp->asc = space_asc;
								sp->desc = space_desc;
								sp->x = x_cursor;

								if (cur_line_tail)
									cur_line_tail->next = sp;
								else
									cur_line_head = sp;
								cur_line_tail = sp;
								x_cursor += space_w;
								line_asc = (int16_t)TEX_MAX(line_asc, space_asc);
								line_desc = (int16_t)TEX_MAX(line_desc, space_desc);
							}
						}
					}

					// wrap check for the math node itself (independent of pending space)
					if (x_cursor + n->w > layout->width && cur_line_head)
					{
						int h = line_asc + line_desc + TEX_LINE_LEADING;
						if (h <= 0)
							h = 1;
						TeX_Line* ln = (TeX_Line*)arena_alloc(&r->arena, sizeof(TeX_Line), sizeof(void*));
						if (ln)
						{
							memset(ln, 0, sizeof(TeX_Line));
							ln->first = cur_line_head;
							ln->y = current_y;
							ln->h = h;
							if (tail_line)
								tail_line->next = ln;
							else
								head = ln;
							tail_line = ln;
							line_count++;
						}
						current_y += h;
						cur_line_head = NULL;
						cur_line_tail = NULL;
						x_cursor = 0;
						line_asc = 0;
						line_desc = 0;
					}

					n->x = x_cursor;

					if (cur_line_tail)
						cur_line_tail->next = n;
					else
						cur_line_head = n;
					cur_line_tail = n;
					x_cursor += n->w;
					line_asc = (int16_t)TEX_MAX(line_asc, n->asc);
					line_desc = (int16_t)TEX_MAX(line_desc, n->desc);
				}
				pending_space = 0;
			}
			break;

		case T_MATH_DISPLAY:
			{
				// finalize current line first
				if (cur_line_head)
				{
					int h = line_asc + line_desc + TEX_LINE_LEADING;
					if (h <= 0)
						h = 1;
					TeX_Line* ln = (TeX_Line*)arena_alloc(&r->arena, sizeof(TeX_Line), sizeof(void*));
					if (ln)
					{
						memset(ln, 0, sizeof(TeX_Line));
						ln->first = cur_line_head;
						ln->y = current_y;
						ln->h = h;
						if (tail_line)
							tail_line->next = ln;
						else
							head = ln;
						tail_line = ln;
						line_count++;
					}
					current_y += h;
					cur_line_head = NULL;
					cur_line_tail = NULL;
					x_cursor = 0;
					line_asc = 0;
					line_desc = 0;
				}

				Node* n = tex_parse_math(t.start, t.len, &r->arena, layout);
				if (n)
				{
					n->flags |= 0x01; // display flag
					tex_measure_node(n, FONTROLE_MAIN);
					int cx = (layout->width - n->w) / 2;
					if (cx < 0)
						cx = 0;
					TEX_COORD_ASSIGN(n->x, cx);

					int h = n->asc + n->desc + TEX_LINE_LEADING;
					TeX_Line* ln = (TeX_Line*)arena_alloc(&r->arena, sizeof(TeX_Line), sizeof(void*));
					if (ln)
					{
						memset(ln, 0, sizeof(TeX_Line));
						ln->first = n;
						ln->y = current_y;
						ln->h = h;
						if (tail_line)
							tail_line->next = ln;
						else
							head = ln;
						tail_line = ln;
						line_count++;
					}
					current_y += h;
				}
				pending_space = 0;
			}
			break;

		default:
			break;
		}
	}

	// finalize any remaining line
	if (cur_line_head)
	{
		int h = line_asc + line_desc + TEX_LINE_LEADING;
		if (h <= 0)
			h = 1;
		TeX_Line* ln = (TeX_Line*)arena_alloc(&r->arena, sizeof(TeX_Line), sizeof(void*));
		if (ln)
		{
			memset(ln, 0, sizeof(TeX_Line));
			ln->first = cur_line_head;
			ln->y = current_y;
			ln->h = h;
			if (tail_line)
				tail_line->next = ln;
			else
				head = ln;
			line_count++;
		}
	}

	r->visible_lines = head;
	r->line_count = line_count;
	r->window_y_start = padded_top;
	r->window_y_end = padded_bot;
	r->cached_layout = layout;
}

void tex_draw(TeX_Renderer* r, TeX_Layout* layout, int x, int y, int scroll_y)
{
	if (!r || !layout)
		return;

#ifdef TEX_DIRECT_RENDER
	g_draw_current_role = (FontRole)-1;
	g_axis_y = 0;
#endif

	int vis_top = 0;
	int vis_bot = TEX_VIEWPORT_H;

	int viewport_top = scroll_y;
	int viewport_bot = scroll_y + TEX_VIEWPORT_H;

	int hit = (r->cached_layout == layout) && (viewport_top >= r->window_y_start) && (viewport_bot <= r->window_y_end);

	if (!hit)
	{
		// miss: rehydrate the visible slice
		rehydrate_window(r, layout, scroll_y);
	}

	// draw from renderer's visible lines
	TeX_Line* lines_to_draw = r->visible_lines;

	// since rehydrate_window already filtered to visible lines within the padded window, just iterate through them. the
	// perline screen position check handles viewport culling
	for (TeX_Line* ln = lines_to_draw; ln; ln = ln->next)
	{
		int line_screen_top = y + (ln->y - scroll_y);
		int line_screen_bot = line_screen_top + ln->h;

		if (line_screen_bot <= vis_top)
			continue;
		if (line_screen_top >= vis_bot)
			break;

		int line_asc = 0;
		int line_desc = 0;
		for (Node* it = ln->first; it; it = it->next)
		{
			line_asc = TEX_MAX(line_asc, it->asc);
			line_desc = TEX_MAX(line_desc, it->desc);
		}
		int baseline_y = line_screen_top + line_asc;

		g_axis_y = baseline_y - tex_metrics_math_axis();

		for (Node* it = ln->first; it; it = it->next)
		{
			int node_x = x + it->x;
			draw_node(it, node_x, baseline_y, FONTROLE_MAIN);
		}

		// the visible_lines linked list is a subset of layout->lines, so we need to stop when we exit the window
		if (r->cached_layout && ln->y + ln->h > r->window_y_end)
			break;
	}
}
