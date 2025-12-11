#include <stdlib.h>
#include <string.h>

#include "tex.h"
#include "tex_internal.h"
#include "tex_measure.h"
#include "tex_metrics.h"
#include "tex_parse.h"
#include "tex_token.h"
#include "tex_util.h"

// default scratch pool size for dryrun math measurement
#define TEX_LAYOUT_SCRATCH_SIZE ((size_t)8 * 1024)

typedef struct
{
	TeX_Layout* L;
	UnifiedPool scratch; // temporary pool for measuring math blocks
	int x_cursor;
	int line_asc;
	int line_desc;
	int pending_space;
	const char* stream_cursor;
	int last_checkpoint_y;
	int has_content;
	int width;
} DryRunState;

static void maybe_record_checkpoint(DryRunState* S)
{
	TeX_Layout* L = S->L;
	if (!L)
		return;

	if (L->total_height - S->last_checkpoint_y < TEX_CHECKPOINT_INTERVAL)
		return;

	if (L->checkpoint_count >= L->checkpoint_capacity)
	{
		int new_cap = L->checkpoint_capacity ? L->checkpoint_capacity * 2 : 8;
		TeX_Checkpoint* new_arr = (TeX_Checkpoint*)realloc(L->checkpoints, (size_t)new_cap * sizeof(TeX_Checkpoint));
		if (!new_arr)
		{
			TEX_SET_ERROR(L, TEX_ERR_OOM, "Failed to grow checkpoint array", new_cap);
			return;
		}
		L->checkpoints = new_arr;
		L->checkpoint_capacity = new_cap;
	}

	L->checkpoints[L->checkpoint_count].y_pos = L->total_height;
	L->checkpoints[L->checkpoint_count].src_ptr = S->stream_cursor;
	L->checkpoint_count++;
	S->last_checkpoint_y = L->total_height;
}

static void finalize_line(DryRunState* S)
{
	if (!S->has_content && S->line_asc == 0 && S->line_desc == 0)
		return;

	int h = S->line_asc + S->line_desc + TEX_LINE_LEADING;
	if (h <= 0)
		h = 1;

	if (S->L->total_height < TEX_MAX_TOTAL_HEIGHT - h)
	{
		S->L->total_height += h;
	}
	else
	{
		S->L->total_height = TEX_MAX_TOTAL_HEIGHT;
		TEX_SET_ERROR(S->L, TEX_ERR_INPUT, "Document height limit exceeded", S->L->total_height);
	}

	S->x_cursor = 0;
	S->line_asc = 0;
	S->line_desc = 0;
	S->pending_space = 0;
	S->has_content = 0;

	maybe_record_checkpoint(S);
}

static void add_content(DryRunState* S, int w, int asc, int desc)
{
	S->line_asc = TEX_MAX(S->line_asc, asc);
	S->line_desc = TEX_MAX(S->line_desc, desc);
	S->x_cursor += w;
	S->has_content = 1;
}

static int check_wrap(DryRunState* S, int w) { return (S->x_cursor + w > S->width) && S->has_content; }

// -------------------------
// Core formatting
// -------------------------

TeX_Layout* tex_format(char* input, int width, TeX_Config* config)
{
	if (!input || width <= 0 || !config)
		return NULL;

	TeX_Layout* L = (TeX_Layout*)calloc(1, sizeof(TeX_Layout));
	if (!L)
		return NULL;

	L->cfg.fg = config->color_fg;
	L->cfg.bg = config->color_bg;
	L->cfg.pack = config->font_pack;
	L->cfg.error_callback = config->error_callback;
	L->cfg.error_userdata = config->error_userdata;
	memset(&L->error, 0, sizeof(L->error));
	L->width = width;
	L->total_height = 0;
	L->source = input;
	L->checkpoints = NULL;
	L->checkpoint_count = 0;
	L->checkpoint_capacity = 0;

#if defined(TEX_DEBUG) && TEX_DEBUG
	L->debug_flags = 0u;
#endif

	tex_metrics_init(L);

	DryRunState st;
	memset(&st, 0, sizeof(st));
	st.L = L;
	st.width = width;
	st.stream_cursor = input;

	if (pool_init(&st.scratch, TEX_LAYOUT_SCRATCH_SIZE) != 0)
	{
		TEX_SET_ERROR(L, TEX_ERR_OOM, "Failed to initialize scratch pool", 0);
		free(L);
		return NULL;
	}

	TeX_Stream stream;
	tex_stream_init(&stream, input, -1);

	TeX_Token t;
	while (tex_stream_next(&stream, &t, &st.scratch, L))
	{
		st.stream_cursor = stream.cursor;

		switch (t.type)
		{
		case T_NEWLINE:
			// For blank lines (no content), set default font height before finalize
			if (!st.has_content && st.line_asc == 0 && st.line_desc == 0)
			{
				st.line_asc = tex_metrics_asc(FONTROLE_MAIN);
				st.line_desc = tex_metrics_desc(FONTROLE_MAIN);
			}
			finalize_line(&st);
			pool_reset(&st.scratch);
			break;

		case T_SPACE:
			st.pending_space = 1;
			pool_reset(&st.scratch);
			break;

		case T_TEXT:
			{
				int text_w = tex_metrics_text_width_n(t.start, t.len, FONTROLE_MAIN);
				int text_asc = tex_metrics_asc(FONTROLE_MAIN);
				int text_desc = tex_metrics_desc(FONTROLE_MAIN);

				if (st.pending_space && st.has_content)
				{
					int space_w = tex_metrics_text_width_n(" ", 1, FONTROLE_MAIN);
					if (check_wrap(&st, space_w + text_w))
					{
						finalize_line(&st);
					}
					else
					{
						add_content(&st, space_w, text_asc, text_desc);
					}
				}
				st.pending_space = 0;

				if (check_wrap(&st, text_w))
				{
					finalize_line(&st);
				}
				add_content(&st, text_w, text_asc, text_desc);
			}

			pool_reset(&st.scratch);
			break;

		case T_MATH_INLINE:
			{
				NodeRef start_node = (NodeRef)st.scratch.node_count;
				NodeRef ref = tex_parse_math(t.start, t.len, &st.scratch, L);
				if (ref != NODE_NULL)
				{
					Node* n = pool_get_node(&st.scratch, ref);
					n->flags &= (uint8_t)~TEX_FLAG_MATHF_DISPLAY;
					tex_measure_range(&st.scratch, start_node, (NodeRef)st.scratch.node_count);

					if (st.pending_space && st.has_content)
					{
						int space_w = tex_metrics_text_width_n(" ", 1, FONTROLE_MAIN);
						if (check_wrap(&st, space_w + n->w))
						{
							finalize_line(&st);
						}
						else
						{
							add_content(&st, space_w, n->asc, n->desc);
						}
					}
					st.pending_space = 0;

					if (check_wrap(&st, n->w))
					{
						finalize_line(&st);
					}
					add_content(&st, n->w, n->asc, n->desc);
				}
				pool_reset(&st.scratch);
			}
			break;

		case T_MATH_DISPLAY:
			{
				finalize_line(&st);

				NodeRef start_node = (NodeRef)st.scratch.node_count;
				NodeRef ref = tex_parse_math(t.start, t.len, &st.scratch, L);
				if (ref != NODE_NULL)
				{
					Node* n = pool_get_node(&st.scratch, ref);
					n->flags |= TEX_FLAG_MATHF_DISPLAY;
					tex_measure_range(&st.scratch, start_node, (NodeRef)st.scratch.node_count);
					add_content(&st, n->w, n->asc, n->desc);
					finalize_line(&st);
				}
				pool_reset(&st.scratch);
			}
			break;

		case T_EOF:
			break;
		}
	}

	finalize_line(&st);

	pool_free(&st.scratch);

	return L;
}

int tex_get_total_height(TeX_Layout* layout)
{
	if (!layout)
		return 0;
	return layout->total_height;
}

void tex_free(TeX_Layout* layout)
{
	if (!layout)
		return;

	free(layout->checkpoints);
	free(layout);
}

TeX_Error tex_get_last_error(TeX_Layout* layout)
{
	if (!layout)
		return TEX_ERR_INPUT;
	return (TeX_Error)layout->error.code;
}

const char* tex_get_error_message(TeX_Layout* layout)
{
	if (!layout)
		return "Invalid layout";
	return layout->error.msg ? layout->error.msg : "";
}

int tex_get_error_value(TeX_Layout* layout)
{
	if (!layout)
		return 0;
	return layout->error.val;
}
