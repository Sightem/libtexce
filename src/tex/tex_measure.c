#include "tex_measure.h"
#include "tex_internal.h"
#include "tex_metrics.h"
#include "tex_util.h"
#include "texfont.h"

// hard cap on how many sibling nodes we will traverse in a single list
#define TEX_MEASURE_LIST_BUDGET 100000

static void measure_text(UnifiedPool* pool, Node* n, FontRole role)
{
	TEX_ASSERT(n != NULL && "measure_text called with NULL node");
	if (n->data.text.len <= 0)
	{
		n->w = 0;
		n->asc = n->desc = 0;
		return;
	}
	const char* str = pool_get_string(pool, n->data.text.sid);
	n->w = tex_metrics_text_width_n(str, n->data.text.len, role);
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
		TEX_COORD_ASSIGN(n->w, n->data.space.em_mul * em);
	}
	else
	{
		TEX_COORD_ASSIGN(n->w, n->data.space.width);
	}
	n->asc = 0;
	n->desc = 0;
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
	case ACC_TILDE:
		return 3;
	case ACC_OVERLINE:
		/* falls through */
	case ACC_UNDERLINE:
		return 1;
	default:
		TEX_ASSERT(0 && "Invalid AccentType in accent_height");
		return 0;
	}
}

static void measure_multiop(Node* n, FontRole role)
{
	TEX_ASSERT(n != NULL && "measure_multiop called with NULL node");
	(void)role;

	// multi ops always use main font like other big operators
	FontRole effective_role = FONTROLE_MAIN;

	uint8_t count = n->data.multiop.count;
	if (count < 1)
		count = 1; // safety

	// get single integral glyph width
	int glyph_w = tex_metrics_glyph_width((unsigned char)TEXFONT_INTEGRAL_CHAR, effective_role);
	int kern = TEX_MULTIOP_KERN;

	// total width: count glyphs + (count-1) kern spaces
	TEX_COORD_ASSIGN(n->w, count * glyph_w + (count - 1) * kern);

	// height matches single integral
	n->asc = tex_metrics_asc(effective_role);
	n->desc = tex_metrics_desc(effective_role);
}

static void aggregate_list(UnifiedPool* pool, ListId head, int* out_w, int* out_asc, int* out_desc)
{
	TEX_ASSERT(out_w != NULL && out_asc != NULL && out_desc != NULL);
	int w = 0, asc = 0, desc = 0;
	int budget = TEX_MEASURE_LIST_BUDGET;

	for (ListId bid = head; bid != LIST_NULL; --budget)
	{
		TEX_ASSERT(budget > 0 && "Budget exhausted: pathologically long list chain");
		TexListBlock* block = pool_get_list_block(pool, bid);
		if (!block)
			break;
		for (uint16_t i = 0; i < block->count; i++)
		{
			Node* n = pool_get_node(pool, block->items[i]);
			if (!n)
				continue;
			// children are already measured in traversal
			w += n->w;
			asc = TEX_MAX(asc, n->asc);
			desc = TEX_MAX(desc, n->desc);
		}
		bid = block->next;
	}
	*out_w = w;
	*out_asc = asc;
	*out_desc = desc;
}

static void measure_node(UnifiedPool* pool, Node* n)
{
	FontRole role = (n->flags & TEX_FLAG_SCRIPT) ? FONTROLE_SCRIPT : FONTROLE_MAIN;

	switch (n->type)
	{
	case N_TEXT:
		measure_text(pool, n, role);
		break;
	case N_GLYPH:
		measure_glyph(n, role);
		break;
	case N_SPACE:
		measure_space(n, role);
		break;
	case N_MATH:
		{
			// aggregate already measured children
			int w = 0, asc = 0, desc = 0;
			aggregate_list(pool, n->data.list.head, &w, &asc, &desc);
			TEX_COORD_ASSIGN(n->w, w);
			TEX_COORD_ASSIGN(n->asc, asc);
			TEX_COORD_ASSIGN(n->desc, desc);
		}
		break;
	case N_SCRIPT:
		{
			// children already measured, compute layout
			Node* base = pool_get_node(pool, n->data.script.base);
			Node* sub = pool_get_node(pool, n->data.script.sub);
			Node* sup = pool_get_node(pool, n->data.script.sup);

			int pad = TEX_SCRIPT_XPAD;
			int w_scripts = 0;
			if (sup)
				w_scripts = TEX_MAX(w_scripts, sup->w);
			if (sub)
				w_scripts = TEX_MAX(w_scripts, sub->w);

			TEX_COORD_ASSIGN(n->w, (base ? base->w : 0) + ((sub || sup) ? (pad + w_scripts) : 0));

			int base_asc = base ? base->asc : 0;
			int base_desc = base ? base->desc : 0;
			int is_bigop = base && tex_node_is_big_operator(base);

			int final_asc = base_asc;
			int final_desc = base_desc;

			if (is_bigop)
			{
				if (sup)
				{
					int sup_h = sup->asc + sup->desc;
					int added = sup_h - TEX_BIGOP_OVERLAP;
					if (added > 0)
						final_asc += added;
				}
				if (sub)
				{
					int sub_h = sub->asc + sub->desc;
					int added = sub_h - TEX_BIGOP_OVERLAP;
					if (added > 0)
						final_desc += added;
				}
			}
			else
			{
				// corner based positioning
				int std_asc = tex_metrics_asc(role);
				int std_desc = tex_metrics_desc(role);

				// default shifts (for small bases like x)
				int def_up = std_asc - (std_asc / 3);
				int def_down = std_desc;

				// off_up: positive pulls inwards (prevents superscript from flying too high)
				// off_down: negative pushes outwards (pulls subscript down below the corner)
				int off_up = std_asc / 2;
				int off_down = -(std_asc / 4);

				// calculate final shifts max(default, corner_target)
				int shift_up = TEX_MAX(def_up, base_asc - off_up);
				int shift_down = TEX_MAX(def_down, base_desc - off_down);

				if (sup)
				{
					int top = shift_up + sup->asc;
					if (top > final_asc)
						final_asc = top;
				}
				if (sub)
				{
					int bot = shift_down + sub->desc;
					if (bot > final_desc)
						final_desc = bot;
				}
			}

			TEX_COORD_ASSIGN(n->asc, final_asc);
			TEX_COORD_ASSIGN(n->desc, final_desc);
		}
		break;
	case N_FRAC:
		{
			Node* num = pool_get_node(pool, n->data.frac.num);
			Node* den = pool_get_node(pool, n->data.frac.den);

			int xpad = TEX_FRAC_XPAD;
			int vpad = TEX_FRAC_YPAD;
			int rule = TEX_RULE_THICKNESS;
			int num_w = num ? num->w : 0;
			int den_w = den ? den->w : 0;
			int inner_w = TEX_MAX(num_w, den_w);

			TEX_COORD_ASSIGN(n->w, inner_w + (2 * xpad) + (2 * TEX_FRAC_OUTER_PAD));

			int num_h = (num ? (num->asc + num->desc) : 0);
			int den_h = (den ? (den->asc + den->desc) : 0);

			int axis = tex_metrics_math_axis();
			TEX_COORD_ASSIGN(n->asc, num_h + vpad + axis);
			TEX_COORD_ASSIGN(n->desc, den_h + vpad + rule - axis);
			if (n->desc < 0)
				n->desc = 0;
		}
		break;
	case N_SQRT:
		{
			Node* r = pool_get_node(pool, n->data.sqrt.rad);
			Node* idx = pool_get_node(pool, n->data.sqrt.index);

			int head_w = tex_metrics_glyph_width((unsigned char)TEXFONT_SQRT_HEAD_CHAR, role);
			int pad = TEX_SQRT_HEAD_XPAD;
			int rad_w = r ? r->w : 0;

			int idx_w = idx ? idx->w : 0;
			int idx_offset = 0;
			if (idx)
			{
				idx_offset = idx_w + TEX_SQRT_INDEX_KERNING;
				if (idx_offset < 0)
					idx_offset = 0;
			}

			TEX_COORD_ASSIGN(n->w, idx_offset + head_w + pad + rad_w);

			int asc_base = tex_metrics_asc(role);
			int rad_asc = r ? r->asc : 0;
			int rad_desc = r ? r->desc : 0;

			int base_asc = TEX_MAX(rad_asc + TEX_ACCENT_GAP, asc_base);

			if (idx)
			{
				int idx_top_dist = (tex_metrics_asc(role) / 2) + idx->asc;
				base_asc = TEX_MAX(base_asc, idx_top_dist);
			}

			TEX_COORD_ASSIGN(n->asc, base_asc);
			TEX_COORD_ASSIGN(n->desc, rad_desc);
		}
		break;
	case N_OVERLAY:
		{
			Node* b = pool_get_node(pool, n->data.overlay.base);
			int ah = accent_height(n->data.overlay.type);
			TEX_COORD_ASSIGN(n->w, b ? b->w : 0);
			int extra = TEX_ACCENT_GAP + ah;
			if (n->data.overlay.type == ACC_UNDERLINE)
			{
				TEX_COORD_ASSIGN(n->asc, b ? b->asc : 0);
				TEX_COORD_ASSIGN(n->desc, (b ? b->desc : 0) + extra);
			}
			else
			{
				TEX_COORD_ASSIGN(n->asc, (b ? b->asc : 0) + extra);
				TEX_COORD_ASSIGN(n->desc, b ? b->desc : 0);
			}
		}
		break;
	case N_SPANDECO:
		{
			Node* content = pool_get_node(pool, n->data.spandeco.content);
			Node* label = pool_get_node(pool, n->data.spandeco.label);

			int bh = TEX_BRACE_HEIGHT;
			int gap = TEX_ACCENT_GAP;

			int w = content ? content->w : 0;
			if (label && label->w > w)
				w = label->w;

			TEX_COORD_ASSIGN(n->w, w);

			if (n->data.spandeco.deco_type == DECO_OVERBRACE)
			{
				int label_h = label ? (label->asc + label->desc + gap) : 0;
				TEX_COORD_ASSIGN(n->asc, (content ? content->asc : 0) + gap + bh + label_h);
				TEX_COORD_ASSIGN(n->desc, content ? content->desc : 0);
			}
			else if (n->data.spandeco.deco_type == DECO_UNDERBRACE)
			{
				int label_h = label ? (label->asc + label->desc + gap) : 0;
				int ub_gap = gap + 2; // extra space above underbrace for tall delimiters
				TEX_COORD_ASSIGN(n->asc, content ? content->asc : 0);
				TEX_COORD_ASSIGN(n->desc, (content ? content->desc : 0) + ub_gap + bh + label_h);
			}
			else if (n->data.spandeco.deco_type == DECO_OVERLINE)
			{
				TEX_COORD_ASSIGN(n->asc, (content ? content->asc : 0) + gap + 1);
				TEX_COORD_ASSIGN(n->desc, content ? content->desc : 0);
			}
			else if (n->data.spandeco.deco_type == DECO_UNDERLINE)
			{
				TEX_COORD_ASSIGN(n->asc, content ? content->asc : 0);
				TEX_COORD_ASSIGN(n->desc, (content ? content->desc : 0) + gap + 1);
			}
		}
		break;
	case N_FUNC_LIM:
		{
			int16_t lim_w = tex_metrics_text_width("lim", FONTROLE_MAIN);
			int16_t lim_asc = tex_metrics_asc(FONTROLE_MAIN);
			int16_t lim_desc = tex_metrics_desc(FONTROLE_MAIN);

			Node* lim = pool_get_node(pool, n->data.func_lim.limit);
			int lim_wid = lim ? lim->w : 0;
			TEX_COORD_ASSIGN(n->w, TEX_MAX(lim_w, lim_wid));
			n->asc = lim_asc;
			TEX_COORD_ASSIGN(n->desc, lim_desc + (lim ? (TEX_FRAC_YPAD + lim->asc + lim->desc) : 0));
		}
		break;
	case N_MULTIOP:
		measure_multiop(n, role);
		break;
	case N_AUTO_DELIM:
		{
			int c_w = 0, c_asc = 0, c_desc = 0;
			NodeRef content_ref = n->data.auto_delim.content;
			if (content_ref != NODE_NULL)
			{
				aggregate_list(pool, content_ref, &c_w, &c_asc, &c_desc);
			}
			else
			{
				c_asc = tex_metrics_asc(role);
				c_desc = tex_metrics_desc(role);
			}

			int axis = tex_metrics_math_axis();
			int dist_up = c_asc - axis;
			int dist_down = c_desc + axis;
			int max_dist = TEX_MAX(dist_up, dist_down);

			int min_dist = (tex_metrics_asc(role) + tex_metrics_desc(role)) / 2;
			max_dist = TEX_MAX(max_dist, min_dist);

			int h = max_dist * 2;
			n->data.auto_delim.delim_h = (int16_t)h;

			int delim_w = h / TEX_DELIM_WIDTH_FACTOR;
			delim_w = TEX_CLAMP(delim_w, TEX_DELIM_MIN_WIDTH, TEX_DELIM_MAX_WIDTH);

			int l_w = (n->data.auto_delim.left_type == DELIM_NONE) ? 0 : delim_w;
			int r_w = (n->data.auto_delim.right_type == DELIM_NONE) ? 0 : delim_w;

			// dynamic kerning for curved parentheses
			// large parentheses have a hollow "belly". To balance spacing relative to the
			// math axis (fraction lines), we overlap the content into this hollow space.
			int kern = delim_w / 2;
			if (n->data.auto_delim.left_type == DELIM_PAREN)
				l_w -= kern;
			if (n->data.auto_delim.right_type == DELIM_PAREN)
				r_w -= kern;

			TEX_COORD_ASSIGN(n->w, l_w + c_w + r_w);
			TEX_COORD_ASSIGN(n->asc, axis + (h / 2));
			TEX_COORD_ASSIGN(n->desc, (h / 2) - axis);
		}
		break;
	case N_MATRIX:
		{
			int16_t col_widths[TEX_MATRIX_MAX_DIMS] = { 0 };
			int16_t row_ascs[TEX_MATRIX_MAX_DIMS] = { 0 };
			int16_t row_descs[TEX_MATRIX_MAX_DIMS] = { 0 };

			uint8_t rows = n->data.matrix.rows;
			uint8_t cols = n->data.matrix.cols;

			if (rows > TEX_MATRIX_MAX_DIMS)
				rows = TEX_MATRIX_MAX_DIMS;
			if (cols > TEX_MATRIX_MAX_DIMS)
				cols = TEX_MATRIX_MAX_DIMS;
			if (cols == 0)
				cols = 1;

			// ollect metrics from all cells
			uint8_t cell_idx = 0;
			for (ListId bid = n->data.matrix.cells; bid != LIST_NULL;)
			{
				TexListBlock* block = pool_get_list_block(pool, bid);
				if (!block)
					break;

				for (uint16_t i = 0; i < block->count; i++)
				{
					uint8_t r = (uint8_t)(cell_idx / cols);
					uint8_t c = (uint8_t)(cell_idx % cols);
					if (r >= rows)
						break;

					Node* cell = pool_get_node(pool, block->items[i]);
					if (cell)
					{
						if (cell->w > col_widths[c])
							col_widths[c] = cell->w;
						if (cell->asc > row_ascs[r])
							row_ascs[r] = cell->asc;
						if (cell->desc > row_descs[r])
							row_descs[r] = cell->desc;
					}
					cell_idx++;
				}
				bid = block->next;
			}

			// sum up dimensions
			int16_t total_w = 0;
			for (uint8_t c = 0; c < cols; c++)
				TEX_COORD_ASSIGN(total_w, total_w + col_widths[c]);
			if (cols > 1)
				TEX_COORD_ASSIGN(total_w, total_w + (cols - 1) * TEX_MATRIX_COL_SPACING);

			// add extra width for column separators (padding on each side of line)
			uint8_t sep_mask = n->data.matrix.col_separators;
			while (sep_mask)
			{
				if (sep_mask & 1)
					TEX_COORD_ASSIGN(total_w, total_w + 2 * TEX_MATRIX_SEP_PAD);
				sep_mask >>= 1;
			}

			int16_t total_h = 0;
			for (uint8_t r = 0; r < rows; r++)
				TEX_COORD_ASSIGN(total_h, total_h + row_ascs[r] + row_descs[r]);
			if (rows > 1)
				TEX_COORD_ASSIGN(total_h, total_h + (rows - 1) * TEX_MATRIX_ROW_SPACING);


			int16_t delim_h = total_h;
			int16_t delim_w = 0;
			if (n->data.matrix.delim_type != DELIM_NONE)
			{
				TEX_COORD_ASSIGN(delim_w, delim_h / TEX_DELIM_WIDTH_FACTOR);
				TEX_COORD_ASSIGN(delim_w, TEX_CLAMP(delim_w, TEX_DELIM_MIN_WIDTH, TEX_DELIM_MAX_WIDTH));
			}

			int16_t axis = tex_metrics_math_axis();
			TEX_COORD_ASSIGN(n->w, total_w + 2 * delim_w);
			TEX_COORD_ASSIGN(n->asc, (total_h / 2) + axis);
			TEX_COORD_ASSIGN(n->desc, total_h - n->asc);
		}
		break;
	default:
		TEX_ASSERT(0 && "Unknown NodeType in measure_node");
		break;
	}
}

void tex_measure_range(UnifiedPool* pool, NodeRef start, NodeRef end)
{
	if (!pool)
		return;

	for (NodeRef i = start; i < end; i++)
	{
		Node* n = pool_get_node(pool, i);
		if (n)
		{
			measure_node(pool, n);
		}
	}
}
