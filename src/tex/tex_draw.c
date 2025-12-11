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

static UnifiedPool* g_draw_pool = NULL;

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
static void draw_node(Node* n, TexCoord x, TexBaseline baseline_y, FontRole role);

static void draw_math_list(NodeRef head, TexCoord x, TexBaseline baseline_y, FontRole role)
{
	TexCoord cur_x = x;
	for (NodeRef it = head; it != NODE_NULL;)
	{
		Node* n = pool_get_node(g_draw_pool, it);
		if (!n)
			break;
		draw_node(n, cur_x, baseline_y, role);
		cur_x.v += n->w;
		it = n->next;
	}
}

static void draw_script(Node* n, TexCoord x, TexBaseline baseline_y, FontRole role)
{
	Node* base = pool_get_node(g_draw_pool, n->data.script.base);
	Node* sub = pool_get_node(g_draw_pool, n->data.script.sub);
	Node* sup = pool_get_node(g_draw_pool, n->data.script.sup);

	if (base)
		draw_node(base, x, baseline_y, role);

	TexCoord script_x = { x.v + (base ? base->w : 0) + TEX_SCRIPT_XPAD };
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
		}
		else if (base->type == N_GLYPH)
		{
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

	if (sup)
	{
		TexBaseline sup_bl;
		if (is_bigop)
		{
			sup_bl.v = (op_top + TEX_BIGOP_OVERLAP) - sup->desc;
		}
		else
		{
			int center_y;
			if (base)
				center_y = baseline_y.v + (base->desc - base->asc) / 2;
			else
				center_y = baseline_y.v - (tex_metrics_asc(role) / 2);

			sup_bl.v = center_y - sup->desc - 1;
		}
		draw_node(sup, script_x, sup_bl, FONTROLE_SCRIPT);
	}

	if (sub)
	{
		TexBaseline sub_bl;
		if (is_bigop)
		{
			sub_bl.v = (op_bot - TEX_BIGOP_OVERLAP) + sub->asc;
		}
		else
		{
			int center_y;
			if (base)
				center_y = baseline_y.v + (base->desc - base->asc) / 2;
			else
				center_y = baseline_y.v - (tex_metrics_asc(role) / 2);

			sub_bl.v = center_y + sub->asc + 1;
		}
		draw_node(sub, script_x, sub_bl, FONTROLE_SCRIPT);
	}
}

static void draw_frac(Node* n, TexCoord x, TexBaseline baseline_y)
{
	Node* num = pool_get_node(g_draw_pool, n->data.frac.num);
	Node* den = pool_get_node(g_draw_pool, n->data.frac.den);

	int axis = tex_metrics_math_axis();
	int rule_y = baseline_y.v - axis;

	int rule_x = x.v + TEX_FRAC_OUTER_PAD;
	int rule_w = n->w - (2 * TEX_FRAC_OUTER_PAD);

	rec_rule(rule_x, rule_y, rule_w);

	if (num)
	{
		TexCoord num_x = { x.v + (n->w - num->w) / 2 };
		TexBaseline num_bl = { rule_y - TEX_FRAC_YPAD - num->desc };
		draw_node(num, num_x, num_bl, FONTROLE_SCRIPT);
	}
	if (den)
	{
		TexCoord den_x = { x.v + (n->w - den->w) / 2 };
		TexBaseline den_bl = { rule_y + TEX_RULE_THICKNESS + TEX_FRAC_YPAD + den->asc };
		draw_node(den, den_x, den_bl, FONTROLE_SCRIPT);
	}
}

static void draw_sqrt(Node* n, TexCoord x, TexBaseline baseline_y, FontRole role)
{
	Node* rad = pool_get_node(g_draw_pool, n->data.sqrt.rad);
	Node* idx = pool_get_node(g_draw_pool, n->data.sqrt.index);

	int head_w = tex_metrics_glyph_width((unsigned char)TEXFONT_SQRT_HEAD_CHAR, role);

	int idx_w = idx ? idx->w : 0;
	int idx_offset = 0;
	if (idx)
	{
		idx_offset = idx_w + TEX_SQRT_INDEX_KERNING;
		if (idx_offset < 0)
			idx_offset = 0;
	}

	int head_x = x.v + idx_offset;
	int head_y_top = baseline_y.v - tex_metrics_asc(role);
	rec_glyph(head_x, head_y_top, (unsigned char)TEXFONT_SQRT_HEAD_CHAR, role);

	if (idx)
	{
		TexCoord idx_x = { x.v };
		TexBaseline idx_bl = { baseline_y.v - (tex_metrics_asc(role) / 2) };
		draw_node(idx, idx_x, idx_bl, FONTROLE_SCRIPT);
	}

	int bar_x = head_x + head_w + TEX_SQRT_HEAD_XPAD;
	if (rad)
	{
		int bar_y = baseline_y.v - rad->asc - TEX_ACCENT_GAP;

		int width = (x.v + n->w) - bar_x;
		rec_line(bar_x, bar_y, bar_x + width, bar_y);
		TexCoord rad_x = { bar_x };
		draw_node(rad, rad_x, baseline_y, role);
	}
}

static void draw_overlay(Node* n, TexCoord x, TexBaseline baseline_y, FontRole role)
{
	Node* b = pool_get_node(g_draw_pool, n->data.overlay.base);

	if (b)
		draw_node(b, x, baseline_y, role);

	if (!b)
		return;

	int top = baseline_y.v - b->asc - TEX_ACCENT_GAP;

	switch (n->data.overlay.type)
	{
	case ACC_BAR:
		{
			int bar_y = top - 1;
			int pad = (b->w > 2) ? 1 : 0;
			rec_line(x.v + pad, bar_y, x.v + b->w - 1 - pad, bar_y);
		}
		break;

	case ACC_OVERLINE:
		{
			int line_y = top - 1;
			int pad = (b->w > 2) ? 1 : 0;
			rec_line(x.v + pad, line_y, x.v + b->w - 1 - pad, line_y);
		}
		break;

	case ACC_UNDERLINE:
		{
			int line_y = baseline_y.v + b->desc + TEX_ACCENT_GAP;
			int pad = (b->w > 2) ? 1 : 0;
			rec_line(x.v + pad, line_y, x.v + b->w - 1 - pad, line_y);
		}
		break;

	case ACC_DOT:
		{
			int cx = x.v + b->w / 2;
			rec_dot(cx, top - 1);
		}
		break;

	case ACC_HAT:
		{
			int cx = x.v + b->w / 2;
			int dy = 3;
			rec_line(cx - dy, top, cx, top - dy);
			rec_line(cx, top - dy, cx + dy, top);
		}
		break;

	case ACC_VEC:
		{
			int len = TEX_MAX(5, b->w);
			int x_end = x.v + b->w;
			int x_start = x_end - len;
			int y = top - 2;

			rec_line(x_start, y, x_end, y);
			rec_line(x_end - 3, y - 2, x_end, y);
			rec_line(x_end - 3, y + 2, x_end, y);
		}
		break;

	case ACC_DDOT:
		{
			int cx = x.v + b->w / 2;
			int sep = 2;
			rec_dot(cx - sep, top - 1);
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
		rec_line(x, y + (is_over ? (TEX_BRACE_HEIGHT - 2) : 1), x + w - 1, y + (is_over ? (TEX_BRACE_HEIGHT - 2) : 1));
		return;
	}
	int mid = x + w / 2;
	int arm_h = 2;
	int top_y = y;
	if (is_over)
	{
		int base_y = top_y + arm_h;
		rec_line(x, base_y, mid - 2, base_y);
		rec_line(mid - 2, base_y, mid, top_y);
		rec_line(mid, top_y, mid + 2, base_y);
		rec_line(mid + 2, base_y, x + w - 1, base_y);
	}
	else
	{
		int base_y = y;
		rec_line(x, base_y, mid - 2, base_y);
		rec_line(mid - 2, base_y, mid, base_y + arm_h + (TEX_BRACE_HEIGHT - arm_h));
		rec_line(mid, base_y + arm_h + (TEX_BRACE_HEIGHT - arm_h), mid + 2, base_y);
		rec_line(mid + 2, base_y, x + w - 1, base_y);
	}
}

static void draw_spandeco(Node* n, TexCoord x, TexBaseline baseline_y, FontRole role)
{
	Node* content = pool_get_node(g_draw_pool, n->data.spandeco.content);
	Node* label = pool_get_node(g_draw_pool, n->data.spandeco.label);
	if (content)
		draw_node(content, x, baseline_y, role);
	int w = content ? content->w : 0;
	int bh = TEX_BRACE_HEIGHT;
	if (n->data.spandeco.deco_type == DECO_OVERBRACE)
	{
		int brace_y = baseline_y.v - (content ? content->asc : 0) - TEX_ACCENT_GAP - bh + 1;
		draw_hbrace(x.v, brace_y, w, 1);
		if (label)
		{
			TexCoord label_x = { x.v + (w - label->w) / 2 };
			TexBaseline label_bl = { brace_y - TEX_ACCENT_GAP - label->desc };
			draw_node(label, label_x, label_bl, FONTROLE_SCRIPT);
		}
	}
	else if (n->data.spandeco.deco_type == DECO_UNDERBRACE)
	{
		int brace_y = baseline_y.v + (content ? content->desc : 0) + TEX_ACCENT_GAP;
		draw_hbrace(x.v, brace_y, w, 0);
		if (label)
		{
			TexCoord label_x = { x.v + (w - label->w) / 2 };
			TexBaseline label_bl = { brace_y + bh + TEX_ACCENT_GAP + label->asc };
			draw_node(label, label_x, label_bl, FONTROLE_SCRIPT);
		}
	}
}

static void draw_multiop(Node* n, TexCoord x)
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

	int cur_x = x.v;
	for (uint8_t i = 0; i < count; i++)
	{
		rec_glyph(cur_x, y_top, (int)TEXFONT_INTEGRAL_CHAR, effective_role);
		cur_x += glyph_w + kern;
	}

	if (n->data.multiop.op_type == MULTIOP_OINT)
	{
		int cx = x.v + (n->w - 1) / 2;
		int cy = g_axis_y + bias;
		int rx = n->w / 2;
		int ry = glyph_w / 2;
		rec_ellipse(cx, cy, rx, ry);
	}
}

static void draw_func_lim(Node* n, TexCoord x, TexBaseline baseline_y)
{
	int y_top = baseline_y.v - tex_metrics_asc(FONTROLE_MAIN);
	rec_text(x.v, y_top, "lim", 3, FONTROLE_MAIN);
	Node* lim = pool_get_node(g_draw_pool, n->data.func_lim.limit);
	if (lim)
	{
		int lim_text_w = tex_metrics_text_width("lim", FONTROLE_MAIN);
		TexCoord lim_x = { x.v + (lim_text_w - lim->w) / 2 };
		TexBaseline lim_bl = { baseline_y.v + TEX_FRAC_YPAD + TEX_RULE_THICKNESS + lim->asc };
		draw_node(lim, lim_x, lim_bl, FONTROLE_SCRIPT);
	}
}

static void draw_proc_delim(int x, int y_center, int h, DelimType type, int is_left)
{
	int delim_w = h / TEX_DELIM_WIDTH_FACTOR;
	delim_w = TEX_CLAMP(delim_w, TEX_DELIM_MIN_WIDTH, TEX_DELIM_MAX_WIDTH);
	int w = delim_w;

	int top = y_center - h / 2;
	int bot = y_center + h / 2;

	switch (type)
	{
	case DELIM_NONE:
		break;
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
			int mid = y_center;
			int cusp = w / 3;
			if (is_left)
			{
				rec_line(x + w - 1, top, x + cusp, top + (mid - top) / 2);
				rec_line(x + cusp, top + (mid - top) / 2, x, mid);
				rec_line(x, mid, x + cusp, mid + (bot - mid) / 2);
				rec_line(x + cusp, mid + (bot - mid) / 2, x + w - 1, bot);
			}
			else
			{
				rec_line(x, top, x + w - 1 - cusp, top + (mid - top) / 2);
				rec_line(x + w - 1 - cusp, top + (mid - top) / 2, x + w - 1, mid);
				rec_line(x + w - 1, mid, x + w - 1 - cusp, mid + (bot - mid) / 2);
				rec_line(x + w - 1 - cusp, mid + (bot - mid) / 2, x, bot);
			}
		}
		break;
	case DELIM_VERT:
		{
			int vx = is_left ? x : (x + w - 1);
			rec_line(vx, top, vx, bot);
		}
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

static void draw_auto_delim(Node* n, TexCoord x, TexBaseline baseline_y, FontRole role)
{
	int h = n->data.auto_delim.delim_h;
	int axis = tex_metrics_math_axis();
	int y_center = baseline_y.v - axis;

	int delim_w = h / TEX_DELIM_WIDTH_FACTOR;
	delim_w = TEX_CLAMP(delim_w, TEX_DELIM_MIN_WIDTH, TEX_DELIM_MAX_WIDTH);

	int l_w = (n->data.auto_delim.left_type == DELIM_NONE) ? 0 : delim_w;
	int r_w = (n->data.auto_delim.right_type == DELIM_NONE) ? 0 : delim_w;

	if (l_w > 0)
		draw_proc_delim(x.v, y_center, h, (DelimType)n->data.auto_delim.left_type, 1);

	NodeRef content_ref = n->data.auto_delim.content;
	if (content_ref != NODE_NULL)
	{
		TexCoord content_x = { x.v + l_w };
		draw_math_list(content_ref, content_x, baseline_y, role);
	}

	if (r_w > 0)
	{
		int c_w = 0;
		for (NodeRef it = content_ref; it != NODE_NULL;)
		{
			Node* cn = pool_get_node(g_draw_pool, it);
			if (!cn)
				break;
			c_w += cn->w;
			it = cn->next;
		}
		int rx = x.v + l_w + c_w;
		draw_proc_delim(rx, y_center, h, (DelimType)n->data.auto_delim.right_type, 0);
	}
}

static void draw_node(Node* n, TexCoord x, TexBaseline baseline_y, FontRole role)
{
	if (!n)
		return;
	switch (n->type)
	{
	case N_TEXT:
		{
			int y_top = baseline_y.v - n->asc;
			const char* s = pool_get_string(g_draw_pool, n->data.text.sid);
			int len = n->data.text.len;
			if (!s || len <= 0)
			{
				s = "";
				len = 0;
			}
			rec_text(x.v, y_top, s, len, role);
		}
		break;
	case N_GLYPH:
		{
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
				rec_glyph(x.v, y_top, (int)n->data.glyph, effective_role);
			}
			else
			{
				int y_top = baseline_y.v - n->asc;
				rec_glyph(x.v, y_top, (int)n->data.glyph, effective_role);
			}
		}
		break;
	case N_SPACE:
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

	pool_reset(&r->pool);
	r->line_count = 0;

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

	NodeRef cur_line_head = NODE_NULL;
	NodeRef cur_line_tail = NODE_NULL;
	int16_t x_cursor = 0;
	int16_t line_asc = 0;
	int16_t line_desc = 0;
	int current_y = y_start;
	int pending_space = 0;

	TeX_Stream stream;
	tex_stream_init(&stream, src_start, -1);

	TeX_Token t;
	while (tex_stream_next(&stream, &t, &r->pool, layout))
	{
		if (current_y >= padded_bot)
			break;
		if (r->line_count >= TEX_RENDERER_MAX_LINES)
			break;

		switch (t.type)
		{
		case T_NEWLINE:
			{
				int eff_asc = line_asc;
				int eff_desc = line_desc;
				if (cur_line_head == NODE_NULL && eff_asc == 0 && eff_desc == 0)
				{
					eff_asc = tex_metrics_asc(FONTROLE_MAIN);
					eff_desc = tex_metrics_desc(FONTROLE_MAIN);
				}

				if (cur_line_head != NODE_NULL || eff_asc > 0 || eff_desc > 0)
				{
					int h = eff_asc + eff_desc + TEX_LINE_LEADING;
					if (h <= 0)
						h = 1;

					if (r->line_count < TEX_RENDERER_MAX_LINES)
					{
						TeX_Line* ln = &r->lines[r->line_count];
						memset(ln, 0, sizeof(TeX_Line));
						ln->first = cur_line_head;
						ln->y = current_y;
						ln->h = h;
						ln->next = NULL;
						r->line_count++;
					}

					current_y += h;
					cur_line_head = NODE_NULL;
					cur_line_tail = NODE_NULL;
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

				if (pending_space && cur_line_head != NODE_NULL)
				{
					int16_t space_w = tex_metrics_text_width_n(" ", 1, FONTROLE_MAIN);
					if (x_cursor + space_w + text_w > layout->width && cur_line_head != NODE_NULL)
					{
						int h = line_asc + line_desc + TEX_LINE_LEADING;
						if (h <= 0)
							h = 1;
						if (r->line_count < TEX_RENDERER_MAX_LINES)
						{
							TeX_Line* ln = &r->lines[r->line_count];
							memset(ln, 0, sizeof(TeX_Line));
							ln->first = cur_line_head;
							ln->y = current_y;
							ln->h = h;
							r->line_count++;
						}
						current_y += h;
						cur_line_head = NODE_NULL;
						cur_line_tail = NODE_NULL;
						x_cursor = 0;
						line_asc = 0;
						line_desc = 0;
					}
					else
					{
						NodeRef sp_ref = pool_alloc_node(&r->pool);
						if (sp_ref != NODE_NULL)
						{
							Node* sp = pool_get_node(&r->pool, sp_ref);
							sp->type = N_TEXT;
							StringId sid = pool_alloc_string(&r->pool, " ", 1);
							sp->data.text.sid = sid;
							sp->data.text.len = 1;
							sp->w = space_w;
							sp->asc = text_asc;
							sp->desc = text_desc;
							sp->x = x_cursor;

							if (cur_line_tail != NODE_NULL)
							{
								Node* tail = pool_get_node(&r->pool, cur_line_tail);
								tail->next = sp_ref;
							}
							else
							{
								cur_line_head = sp_ref;
							}
							cur_line_tail = sp_ref;
							x_cursor = (int16_t)(x_cursor + space_w);
							line_asc = TEX_MAX(line_asc, text_asc);
							line_desc = TEX_MAX(line_desc, text_desc);
						}
					}
				}
				pending_space = 0;

				if (x_cursor + text_w > layout->width && cur_line_head != NODE_NULL)
				{
					int h = line_asc + line_desc + TEX_LINE_LEADING;
					if (h <= 0)
						h = 1;
					if (r->line_count < TEX_RENDERER_MAX_LINES)
					{
						TeX_Line* ln = &r->lines[r->line_count];
						memset(ln, 0, sizeof(TeX_Line));
						ln->first = cur_line_head;
						ln->y = current_y;
						ln->h = h;
						r->line_count++;
					}
					current_y += h;
					cur_line_head = NODE_NULL;
					cur_line_tail = NODE_NULL;
					x_cursor = 0;
					line_asc = 0;
					line_desc = 0;
				}

				NodeRef node_ref = pool_alloc_node(&r->pool);
				if (node_ref != NODE_NULL)
				{
					Node* node = pool_get_node(&r->pool, node_ref);
					node->type = N_TEXT;
					StringId sid = pool_alloc_string(&r->pool, t.start, (size_t)t.len);
					node->data.text.sid = sid;
					node->data.text.len = (uint16_t)t.len;
					node->w = text_w;
					node->asc = text_asc;
					node->desc = text_desc;
					node->x = x_cursor;

					if (cur_line_tail != NODE_NULL)
					{
						Node* tail = pool_get_node(&r->pool, cur_line_tail);
						tail->next = node_ref;
					}
					else
					{
						cur_line_head = node_ref;
					}
					cur_line_tail = node_ref;
					x_cursor = (int16_t)(x_cursor + text_w);
					line_asc = TEX_MAX(line_asc, text_asc);
					line_desc = TEX_MAX(line_desc, text_desc);
				}
			}
			break;

		case T_MATH_INLINE:
			{
				NodeRef start_node = (NodeRef)r->pool.node_count;
				NodeRef math_ref = tex_parse_math(t.start, t.len, &r->pool, layout);
				if (math_ref != NODE_NULL)
				{
					Node* math = pool_get_node(&r->pool, math_ref);
					tex_measure_range(&r->pool, start_node, (NodeRef)r->pool.node_count);

					if (pending_space && cur_line_head != NODE_NULL)
					{
						int16_t space_w = tex_metrics_text_width_n(" ", 1, FONTROLE_MAIN);
						if (x_cursor + space_w + math->w > layout->width && cur_line_head != NODE_NULL)
						{
							int h = line_asc + line_desc + TEX_LINE_LEADING;
							if (h <= 0)
								h = 1;
							if (r->line_count < TEX_RENDERER_MAX_LINES)
							{
								TeX_Line* ln = &r->lines[r->line_count];
								memset(ln, 0, sizeof(TeX_Line));
								ln->first = cur_line_head;
								ln->y = current_y;
								ln->h = h;
								r->line_count++;
							}
							current_y += h;
							cur_line_head = NODE_NULL;
							cur_line_tail = NODE_NULL;
							x_cursor = 0;
							line_asc = 0;
							line_desc = 0;
						}
						else
						{
							x_cursor = (int16_t)(x_cursor + space_w);
						}
					}
					pending_space = 0;

					if (x_cursor + math->w > layout->width && cur_line_head != NODE_NULL)
					{
						int h = line_asc + line_desc + TEX_LINE_LEADING;
						if (h <= 0)
							h = 1;
						if (r->line_count < TEX_RENDERER_MAX_LINES)
						{
							TeX_Line* ln = &r->lines[r->line_count];
							memset(ln, 0, sizeof(TeX_Line));
							ln->first = cur_line_head;
							ln->y = current_y;
							ln->h = h;
							r->line_count++;
						}
						current_y += h;
						cur_line_head = NODE_NULL;
						cur_line_tail = NODE_NULL;
						x_cursor = 0;
						line_asc = 0;
						line_desc = 0;
					}

					math->x = x_cursor;
					if (cur_line_tail != NODE_NULL)
					{
						Node* tail = pool_get_node(&r->pool, cur_line_tail);
						tail->next = math_ref;
					}
					else
					{
						cur_line_head = math_ref;
					}
					cur_line_tail = math_ref;
					x_cursor = (int16_t)(x_cursor + math->w);
					line_asc = TEX_MAX(line_asc, math->asc);
					line_desc = TEX_MAX(line_desc, math->desc);
				}
			}
			break;

		case T_MATH_DISPLAY:
			{
				if (cur_line_head != NODE_NULL)
				{
					int h = line_asc + line_desc + TEX_LINE_LEADING;
					if (h <= 0)
						h = 1;
					if (r->line_count < TEX_RENDERER_MAX_LINES)
					{
						TeX_Line* ln = &r->lines[r->line_count];
						memset(ln, 0, sizeof(TeX_Line));
						ln->first = cur_line_head;
						ln->y = current_y;
						ln->h = h;
						r->line_count++;
					}
					current_y += h;
					cur_line_head = NODE_NULL;
					cur_line_tail = NODE_NULL;
					x_cursor = 0;
					line_asc = 0;
					line_desc = 0;
				}

				NodeRef start_node = (NodeRef)r->pool.node_count;
				NodeRef math_ref = tex_parse_math(t.start, t.len, &r->pool, layout);
				if (math_ref != NODE_NULL)
				{
					Node* math = pool_get_node(&r->pool, math_ref);
					tex_measure_range(&r->pool, start_node, (NodeRef)r->pool.node_count);

					// Center display math
					int16_t center_x = (int16_t)((layout->width - math->w) / 2);
					if (center_x < 0)
						center_x = 0;
					math->x = center_x;
					cur_line_head = math_ref;
					cur_line_tail = math_ref;
					line_asc = math->asc;
					line_desc = math->desc;

					int h = line_asc + line_desc + TEX_LINE_LEADING;
					if (h <= 0)
						h = 1;
					if (r->line_count < TEX_RENDERER_MAX_LINES)
					{
						TeX_Line* ln = &r->lines[r->line_count];
						memset(ln, 0, sizeof(TeX_Line));
						ln->first = cur_line_head;
						ln->y = current_y;
						ln->h = h;
						r->line_count++;
					}
					current_y += h;
					cur_line_head = NODE_NULL;
					cur_line_tail = NODE_NULL;
					x_cursor = 0;
					line_asc = 0;
					line_desc = 0;
				}
				pending_space = 0;
			}
			break;

		case T_EOF:
			break;
		}
	}

	if (cur_line_head != NODE_NULL && r->line_count < TEX_RENDERER_MAX_LINES)
	{
		int h = line_asc + line_desc + TEX_LINE_LEADING;
		if (h <= 0)
			h = 1;
		TeX_Line* ln = &r->lines[r->line_count];
		memset(ln, 0, sizeof(TeX_Line));
		ln->first = cur_line_head;
		ln->y = current_y;
		ln->h = h;
		r->line_count++;
	}

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
		rehydrate_window(r, layout, scroll_y);
	}

	// pool context for draw functions
	g_draw_pool = &r->pool;

	for (int i = 0; i < r->line_count; i++)
	{
		TeX_Line* ln = &r->lines[i];
		int line_screen_top = y + (ln->y - scroll_y);
		int line_screen_bot = line_screen_top + ln->h;

		if (line_screen_bot <= vis_top)
			continue;
		if (line_screen_top >= vis_bot)
			break;

		int line_asc = 0;
		int line_desc = 0;
		for (NodeRef it = ln->first; it != NODE_NULL;)
		{
			Node* n = pool_get_node(g_draw_pool, it);
			if (!n)
				break;
			line_asc = TEX_MAX(line_asc, n->asc);
			line_desc = TEX_MAX(line_desc, n->desc);
			it = n->next;
		}
		int baseline = line_screen_top + line_asc;
		TexBaseline baseline_y = { baseline };

		g_axis_y = baseline - tex_metrics_math_axis();

		for (NodeRef it = ln->first; it != NODE_NULL;)
		{
			Node* n = pool_get_node(g_draw_pool, it);
			if (!n)
				break;
			TexCoord node_x = { x + n->x };
			draw_node(n, node_x, baseline_y, FONTROLE_MAIN);
			it = n->next;
		}

		if (r->cached_layout && ln->y + ln->h > r->window_y_end)
			break;
	}

	g_draw_pool = NULL;
}
