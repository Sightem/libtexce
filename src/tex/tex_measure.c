#include "tex_measure.h"
#include "tex_internal.h"
#include "tex_metrics.h"
#include "tex_util.h"
#include "texfont.h"


// guard to prevent pathological recursion/cycles during measurement
// uses the high bit of Node.flags which is otherwise unused by measurement
#define TEX_FLAG_MEAS_VISITING 0x80

// hard cap on how many sibling nodes we will traverse in a single list
#define TEX_MEASURE_LIST_BUDGET 100000

static void measure_list(Node* head, FontRole role, int* out_w, int* out_asc, int* out_desc);

static void measure_text(Node* n, FontRole role)
{
	TEX_ASSERT(n != NULL && "measure_text called with NULL node");
	if (!n->data.text.ptr || n->data.text.len <= 0)
	{
		n->w = 0;
		n->asc = n->desc = 0;
		return;
	}
	n->w = tex_metrics_text_width_n(n->data.text.ptr, n->data.text.len, role);
	n->asc = tex_metrics_asc(role);
	n->desc = tex_metrics_desc(role);
}

static void measure_glyph(Node* n, FontRole role)
{
	TEX_ASSERT(n != NULL && "measure_glyph called with NULL node");
	// big operators (integral, summation, product) always use main font
	// to maintain legibility even in nested contexts like fractions
	FontRole effective_role = role;
	if (tex_is_big_operator(n->data.glyph))
	{
		effective_role = FONTROLE_MAIN;
	}
	// use actual glyph width if available, else fallback CW
	n->w = tex_metrics_glyph_width(n->data.glyph, effective_role);
	n->asc = tex_metrics_asc(effective_role);
	n->desc = tex_metrics_desc(effective_role);
}

static void measure_space(Node* n, FontRole role)
{
	TEX_ASSERT(n != NULL && "measure_space called with NULL node");
	if (n->data.space.em_mul)
	{
		int em = tex_metrics_asc(role) + tex_metrics_desc(role);
		n->w = n->data.space.em_mul * em;
	}
	else
	{
		n->w = n->data.space.width;
	}
	n->asc = 0;
	n->desc = 0;
}

static void measure_math(Node* n, FontRole role)
{
	TEX_ASSERT(n != NULL && "measure_math called with NULL node");
	int w = 0, asc = 0, desc = 0;
	measure_list(n->child, role, &w, &asc, &desc);
	n->w = w;
	n->asc = asc;
	n->desc = desc;
}

static void measure_script(Node* n, FontRole role)
{
	TEX_ASSERT(n != NULL && "measure_script called with NULL node");
	Node* base = n->data.script.base;
	Node* sub = n->data.script.sub;
	Node* sup = n->data.script.sup;

	if (base)
	{
		tex_measure_node(base, role);
	}
	if (sub)
	{
		tex_measure_node(sub, FONTROLE_SCRIPT);
	}
	if (sup)
	{
		tex_measure_node(sup, FONTROLE_SCRIPT);
	}

	int pad = TEX_SCRIPT_XPAD;
	int w_scripts = 0;
	if (sup)
	{
		w_scripts = TEX_MAX(w_scripts, sup->w);
	}
	if (sub)
	{
		w_scripts = TEX_MAX(w_scripts, sub->w);
	}

	n->w = (base ? base->w : 0) + ((sub || sup) ? (pad + w_scripts) : 0);

	// vertical extents
	int base_asc = base ? base->asc : 0;
	int base_desc = base ? base->desc : 0;
	int is_bigop = base && tex_node_is_big_operator(base);

	int final_asc = base_asc;
	int final_desc = base_desc;

	if (is_bigop)
	{
		// big operators:
		// the scripts are positioned relative to the total height
		// since we overlap, we simply add the script height minus the overlap
		if (sup)
		{
			int sup_h = sup->asc + sup->desc;

			// effective addition to ascender
			int added = sup_h - TEX_BIGOP_OVERLAP;
			if (added > 0)
			{
				final_asc += added;
			}
		}
		if (sub)
		{
			int sub_h = sub->asc + sub->desc;

			int added = sub_h - TEX_BIGOP_OVERLAP;
			if (added > 0)
			{
				final_desc += added;
			}
		}
	}
	else
	{
		// normal text
		int delta_y;
		if (base)
		{
			delta_y = (base_desc - base_asc) / 2;
		}
		else
		{
			delta_y = (tex_metrics_desc(role) - tex_metrics_asc(role)) / 2;
		}

		// Sup Top is (sup_height + 1) above center
		// center is (baseline + delta_y)
		// distance from baseline to Sup Top = (sup_h + 1) - delta_y
		if (sup)
		{
			int h = sup->asc + sup->desc;
			int dist = h + 1 - delta_y;
			if (dist > final_asc)
			{
				final_asc = dist;
			}
		}

		// sub bot is (sub_height + 1) below center
		// distance from baseline to sub bot = (sub_h + 1) + delta_y
		if (sub)
		{
			int h = sub->asc + sub->desc;
			int dist = h + 1 + delta_y;
			if (dist > final_desc)
			{
				final_desc = dist;
			}
		}
	}

	n->asc = final_asc;
	n->desc = final_desc;
}

static void measure_frac(Node* n, FontRole role)
{
	TEX_ASSERT(n != NULL && "measure_frac called with NULL node");
	(void)role;
	Node* num = n->data.frac.num;
	Node* den = n->data.frac.den;
	if (num)
	{
		tex_measure_node(num, FONTROLE_SCRIPT);
	}
	if (den)
	{
		tex_measure_node(den, FONTROLE_SCRIPT);
	}

	int xpad = TEX_FRAC_XPAD;
	int vpad = TEX_FRAC_YPAD;
	int rule = TEX_RULE_THICKNESS;
	int num_w = num ? num->w : 0;
	int den_w = den ? den->w : 0;
	int inner_w = TEX_MAX(num_w, den_w);

	// include horizontal outer padding around the rule
	n->w = inner_w + (2 * xpad) + (2 * TEX_FRAC_OUTER_PAD);

	int num_h = (num ? (num->asc + num->desc) : 0);
	int den_h = (den ? (den->asc + den->desc) : 0);

	int axis = tex_metrics_math_axis();
	n->asc = num_h + vpad + axis;
	n->desc = den_h + vpad + rule - axis;
	if (n->desc < 0)
	{
		n->desc = 0;
	}
}

static void measure_sqrt(Node* n, FontRole role)
{
	TEX_ASSERT(n != NULL && "measure_sqrt called with NULL node");
	Node* r = n->data.sqrt.rad;
	Node* idx = n->data.sqrt.index;

	if (r)
	{
		tex_measure_node(r, role);
	}

	if (idx)
	{
		tex_measure_node(idx, FONTROLE_SCRIPT);
	}

	int head_w = tex_metrics_glyph_width((unsigned char)TEXFONT_SQRT_HEAD_CHAR, role);
	int pad = TEX_SQRT_HEAD_XPAD;
	int rad_w = r ? r->w : 0;

	// calculate horizontal offset for the radical to make room for the index
	// shift radical right by (index_width + kerning)
	// this creates a gap or controlled overlap defined by TEX_SQRT_INDEX_KERNING
	int idx_w = idx ? idx->w : 0;
	int idx_offset = 0;
	if (idx)
	{
		idx_offset = idx_w + TEX_SQRT_INDEX_KERNING;
		if (idx_offset < 0)
		{
			idx_offset = 0;
		}
	}

	// total width is the offset start of the radical + the radical's own width
	// radical width = head + pad + radicand
	n->w = idx_offset + head_w + pad + rad_w;

	// vertical metrics
	int asc_base = tex_metrics_asc(role);
	int rad_asc = r ? r->asc : 0;
	int rad_desc = r ? r->desc : 0;

	int base_asc = TEX_MAX(rad_asc + TEX_ACCENT_GAP, asc_base);

	// if index present, ensure bounding box covers it
	if (idx)
	{
		int idx_top_dist = (tex_metrics_asc(role) / 2) + idx->asc;
		base_asc = TEX_MAX(base_asc, idx_top_dist);
	}

	n->asc = base_asc;
	n->desc = rad_desc;
}
static int accent_height(uint8_t type)
{
	switch (type)
	{
	case ACC_BAR:
		return 1;
	case ACC_DOT:
		return 2;
	case ACC_HAT:
		return 3;
	case ACC_VEC:
		return 4;
	case ACC_DDOT:
		return 2;
	case ACC_OVERLINE:
		__attribute__((fallthrough));
	case ACC_UNDERLINE:
		return 1;
	default:
		TEX_ASSERT(0 && "Invalid AccentType in accent_height");
		return 0;
	}
}

static void measure_overlay(Node* n, FontRole role)
{
	TEX_ASSERT(n != NULL && "measure_overlay called with NULL node");
	Node* b = n->data.overlay.base;
	if (b)
		tex_measure_node(b, role);
	int ah = accent_height(n->data.overlay.type);
	n->w = b ? b->w : 0;
	int extra = TEX_ACCENT_GAP + ah;
	if (n->data.overlay.type == ACC_UNDERLINE)
	{
		// underline extends into descender region
		n->asc = b ? b->asc : 0;
		n->desc = (b ? b->desc : 0) + extra;
	}
	else
	{
		// over accents extend ascender
		n->asc = (b ? b->asc : 0) + extra;
		n->desc = b ? b->desc : 0;
	}
}

static void measure_spandeco(Node* n, FontRole role)
{
	TEX_ASSERT(n != NULL && "measure_spandeco called with NULL node");
	Node* content = n->data.spandeco.content;
	Node* label = n->data.spandeco.label;
	if (content)
		tex_measure_node(content, role);
	if (label)
		tex_measure_node(label, FONTROLE_SCRIPT);

	int bh = TEX_BRACE_HEIGHT;
	int gap = TEX_ACCENT_GAP;

	int w = content ? content->w : 0;
	if (label && label->w > w)
		w = label->w;

	n->w = w;

	if (n->data.spandeco.deco_type == DECO_OVERBRACE)
	{
		int label_h = label ? (label->asc + label->desc + gap) : 0;
		n->asc = (content ? content->asc : 0) + gap + bh + label_h;
		n->desc = content ? content->desc : 0;
	}
	else if (n->data.spandeco.deco_type == DECO_UNDERBRACE)
	{
		int label_h = label ? (label->asc + label->desc + gap) : 0;
		n->asc = content ? content->asc : 0;
		n->desc = (content ? content->desc : 0) + gap + bh + label_h;
	}
	else if (n->data.spandeco.deco_type == DECO_OVERLINE)
	{
		// neither of these are currently produced by parser
		n->asc = (content ? content->asc : 0) + gap + 1;
		n->desc = content ? content->desc : 0;
	}
	else if (n->data.spandeco.deco_type == DECO_UNDERLINE)
	{
		n->asc = content ? content->asc : 0;
		n->desc = (content ? content->desc : 0) + gap + 1;
	}
}

static void measure_func_lim(Node* n, FontRole role)
{
	TEX_ASSERT(n != NULL && "measure_func_lim called with NULL node");
	(void)role; // unused: lim always uses FONTROLE_MAIN

	// measure "lim" text in main role
	int lim_w = tex_metrics_text_width("lim", FONTROLE_MAIN);
	int lim_asc = tex_metrics_asc(FONTROLE_MAIN);
	int lim_desc = tex_metrics_desc(FONTROLE_MAIN);
	Node* lim = n->data.func_lim.limit;
	if (lim)
	{
		tex_measure_node(lim, FONTROLE_SCRIPT);
	}
	int lim_wid = lim ? lim->w : 0;
	n->w = TEX_MAX(lim_w, lim_wid);
	n->asc = lim_asc;
	n->desc = lim_desc + (lim ? (TEX_FRAC_YPAD + lim->asc + lim->desc) : 0);
}

static void measure_list(Node* head, FontRole role, int* out_w, int* out_asc, int* out_desc)
{
	TEX_ASSERT(out_w != NULL && out_asc != NULL && out_desc != NULL);
	int w = 0, asc = 0, desc = 0;

	// TODO: guard this in debug macros? it really shouldnt be required in release
	int has_cycle = 0;
	{
		Node* slow = head;
		Node* fast = head;
		while (fast && fast->next)
		{
			slow = slow->next;
			fast = fast->next->next;
			if (slow == fast)
			{
				has_cycle = 1;
				break;
			}
		}
	}

	if (has_cycle)
	{
		TEX_ASSERT(0 && "FATAL: Cycle detected in node list—parser or arena corruption");
	}

	int budget = TEX_MEASURE_LIST_BUDGET;

	for (Node* it = head; it; it = it->next, --budget)
	{
		TEX_ASSERT(budget > 0 && "Budget exhausted: pathologically long node list");
		tex_measure_node(it, role);
		w += it->w;
		asc = TEX_MAX(asc, it->asc);
		desc = TEX_MAX(desc, it->desc);
	}
	*out_w = w;
	*out_asc = asc;
	*out_desc = desc;
}

static void measure_multiop(Node* n, FontRole role)
{
	TEX_ASSERT(n != NULL && "measure_multiop called with NULL node");
	(void)role;

	// multi ops always use main font like other big operators
	FontRole effective_role = FONTROLE_MAIN;

	uint8_t count = n->data.multiop.count;
	if (count < 1)
		count = 1; // Safety

	// get single integral glyph width
	int glyph_w = tex_metrics_glyph_width((unsigned char)TEXFONT_INTEGRAL_CHAR, effective_role);
	int kern = TEX_MULTIOP_KERN;

	// total width: count glyphs + (count-1) kern spaces
	n->w = count * glyph_w + (count - 1) * kern;

	// height matches single integral
	n->asc = tex_metrics_asc(effective_role);
	n->desc = tex_metrics_desc(effective_role);
}

static void measure_auto_delim(Node* n, FontRole role)
{
	TEX_ASSERT(n != NULL && "measure_auto_delim called with NULL node");

	// Measure content first
	Node* content = n->data.auto_delim.content;
	int c_w = 0, c_asc = 0, c_desc = 0;
	if (content)
	{
		measure_list(content, role, &c_w, &c_asc, &c_desc);
	}
	else
	{
		c_asc = tex_metrics_asc(role);
		c_desc = tex_metrics_desc(role);
	}

	// calculate symmetric vertical extent around math axis
	int axis = tex_metrics_math_axis();
	int dist_up = c_asc - axis;
	int dist_down = c_desc + axis;
	int max_dist = TEX_MAX(dist_up, dist_down);

	int min_dist = (tex_metrics_asc(role) + tex_metrics_desc(role)) / 2;
	max_dist = TEX_MAX(max_dist, min_dist);

	// total symmetric height
	int h = max_dist * 2;
	n->data.auto_delim.delim_h = (int16_t)h;

	// delimiter widths
	int delim_w = h / TEX_DELIM_WIDTH_FACTOR;
	delim_w = TEX_CLAMP(delim_w, TEX_DELIM_MIN_WIDTH, TEX_DELIM_MAX_WIDTH);

	int l_w = (n->data.auto_delim.left_type == DELIM_NONE) ? 0 : delim_w;
	int r_w = (n->data.auto_delim.right_type == DELIM_NONE) ? 0 : delim_w;

	// node metrics
	n->w = l_w + c_w + r_w;
	n->asc = axis + (h / 2);
	n->desc = (h / 2) - axis;
}

void tex_measure_node(Node* n, FontRole role)
{
	if (!n)
		return;

	// if a node is encountered while it is already being measured (due to a malformed cycle in the graph), stop
	// recursion
	if (n->flags & TEX_FLAG_MEAS_VISITING)
	{
		TEX_ASSERT(0 && "FATAL: Re-entrant measurement detected—cycle in node graph");
	}
	// mark as visiting for the duration of this call
	n->flags |= TEX_FLAG_MEAS_VISITING;
	switch (n->type)
	{
	case N_TEXT:
		measure_text(n, role);
		break;
	case N_GLYPH:
		measure_glyph(n, role);
		break;
	case N_SPACE:
		measure_space(n, role);
		break;
	case N_MATH:
		measure_math(n, role);
		break;
	case N_SCRIPT:
		measure_script(n, role);
		break;
	case N_FRAC:
		measure_frac(n, role);
		break;
	case N_SQRT:
		measure_sqrt(n, role);
		break;
	case N_OVERLAY:
		measure_overlay(n, role);
		break;
	case N_SPANDECO:
		measure_spandeco(n, role);
		break;
	case N_FUNC_LIM:
		measure_func_lim(n, role);
		break;
	case N_MULTIOP:
		measure_multiop(n, role);
		break;
	case N_AUTO_DELIM:
		measure_auto_delim(n, role);
		break;
	default:
		TEX_ASSERT(0 && "Unknown NodeType in tex_measure_node");
		break;
	}

	n->flags &= (uint8_t)~TEX_FLAG_MEAS_VISITING;
}
