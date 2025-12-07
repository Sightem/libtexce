#include <stdlib.h>
#include <string.h>

#include "tex.h"
#include "tex_arena.h"
#include "tex_internal.h"
#include "tex_measure.h"
#include "tex_metrics.h"
#include "tex_parse.h"
#include "tex_token.h"
#include "tex_util.h"

// flags for Node.flags used locally for N_MATH style
#define MATHF_DISPLAY 0x01

// -------------------------
// small helpers
// -------------------------
static Node* ln_new_node(TeX_Layout* L, NodeType t)
{
	if (TEX_HAS_ERROR(L))
	{
		return NULL;
	}

	Node* n = (Node*)arena_alloc(&L->arena, sizeof(Node), sizeof(void*));
	if (!n)
	{
		TEX_SET_ERROR(L, TEX_ERR_OOM, "Failed to allocate layout node", 0);
		return NULL;
	}
	memset(n, 0, sizeof(Node));
	n->type = (uint8_t)t;
	return n;
}

static TeX_Line* new_line(TexArena* A)
{
	TeX_Line* ln = (TeX_Line*)arena_alloc(A, sizeof(TeX_Line), sizeof(void*));
	if (!ln)
	{
		return NULL;
	}
	memset(ln, 0, sizeof(TeX_Line));
	return ln;
}

static void append_line(TeX_Layout* L, TeX_Line* ln)
{
	TEX_ASSERT(ln != NULL && "append_line called with NULL line");
	if (!L->lines)
	{
		L->lines = ln;
		return;
	}
	TeX_Line* t = L->lines;
	while (t->next)
	{
		t = t->next;
	}
	t->next = ln;
}

static void build_line_index(TeX_Layout* L)
{
	TEX_ASSERT(L != NULL && "build_line_index called with NULL Layout");
	// count lines
	int count = 0;
	const int MAX_LINES = 100000;
	for (TeX_Line* ln = L->lines; ln && count < MAX_LINES; ln = ln->next)
		count++;
	TEX_ASSERT(count < MAX_LINES && "Line list appears cyclic or excessively long");
	L->line_count = count;
	// small documents do not need an index
	if (count <= 16)
	{
		L->line_index = NULL;
		return;
	}

	// allocate index array from arena
	TeX_Line** arr = (TeX_Line**)arena_alloc(&L->arena, (size_t)count * sizeof(TeX_Line*), sizeof(void*));
	if (!arr)
	{
		L->line_index = NULL;
		TEX_SET_ERROR(L, TEX_ERR_OOM, "Failed to allocate line index", count);
		return;
	}
	int i = 0;
	for (TeX_Line* ln = L->lines; ln; ln = ln->next)
	{
		arr[i++] = ln;
	}
	L->line_index = arr;
}

// postline coalescing of adjacent N_TEXT nodes
static void coalesce_line_text_nodes(TeX_Line* line, TexArena* arena, TeX_Layout* L)
{
	if (!line || !line->first)
	{
		return;
	}

	// fast path: if no adjacent N_TEXT nodes, short out
	int need_merge = 0;
	for (Node* c = line->first; c && c->next; c = c->next)
	{
		if (c->type == N_TEXT && c->next->type == N_TEXT)
		{
			need_merge = 1;
			break;
		}
	}
	if (!need_merge)
	{
		return;
	}

	Node* prev = NULL;
	Node* cur = line->first;
	while (cur)
	{
		if (cur->type != N_TEXT)
		{
			prev = cur;
			cur = cur->next;
			continue;
		}

		// check if theres a run of adjacent N_TEXT nodes to merge
		Node* run_start = cur;
		Node* run_end = cur;
		int run_count = 1;

		while (run_end->next && run_end->next->type == N_TEXT)
		{
			run_end = run_end->next;
			++run_count;
		}

		// skip if nothing to coalesce (inline coalescing already handles most cases)
		if (run_count == 1)
		{
			prev = cur;
			cur = cur->next;
			continue;
		}

		// calculate totals for the run
		int total_len = 0;
		int sum_w = 0;
		int max_asc = 0;
		int max_desc = 0;
		for (Node* it = run_start; it; it = it->next)
		{
			if (it->data.text.len > 0)
			{
				total_len += it->data.text.len;
			}
			sum_w += it->w;
			max_asc = TEX_MAX(max_asc, it->asc);
			max_desc = TEX_MAX(max_desc, it->desc);
			if (it == run_end)
			{
				break;
			}
		}

		// check zero copy feasibility
		int can_zc = 1;
		for (Node* it = run_start; it && it != run_end; it = it->next)
		{
			const char* a_ptr = it->data.text.ptr;
			int a_len = it->data.text.len;
			const char* b_ptr = it->next ? it->next->data.text.ptr : NULL;
			if (!(a_ptr && b_ptr && a_len >= 0))
			{
				can_zc = 0;
				break;
			}
			if (a_ptr + a_len != b_ptr)
			{
				can_zc = 0;
				break;
			}
		}

		if (can_zc)
		{
			// zero copy: expand first node to span entire run
			const char* start = run_start->data.text.ptr;
			const char* end = run_end->data.text.ptr + run_end->data.text.len;
			run_start->data.text.ptr = start;
			run_start->data.text.len = (int)(end - start);
		}
		else
		{
			// fallback: alloc and copy TODO: instrument this to see when it happens, should be rare
			char* buf = (char*)arena_alloc(arena, (size_t)total_len + 1, 1);
			if (!buf)
			{
				TEX_SET_ERROR(L, TEX_ERR_OOM, "Failed to coalesce text nodes", 0);
				prev = run_end;
				cur = run_end->next;
				continue;
			}

			int off = 0;
			for (Node* it = run_start;; it = it->next)
			{
				if (it->data.text.ptr && it->data.text.len > 0)
				{
					memcpy(buf + off, it->data.text.ptr, (size_t)it->data.text.len);
					off += it->data.text.len;
				}
				if (it == run_end)
				{
					break;
				}
			}
			buf[off] = '\0';

			run_start->data.text.ptr = buf;
			run_start->data.text.len = total_len;
		}

		run_start->w = sum_w;
		run_start->asc = max_asc;
		run_start->desc = max_desc;
		run_start->next = run_end->next;

		if (!prev)
		{
			line->first = run_start;
		}
		else
		{
			prev->next = run_start;
		}

		line->child_count -= (run_count - 1);

		prev = run_start;
		cur = run_start->next;
	}
}
static void coalesce_all_lines(TeX_Layout* L)
{
	if (!L)
	{
		return;
	}
	for (TeX_Line* ln = L->lines; ln; ln = ln->next)
		coalesce_line_text_nodes(ln, &L->arena, L);
}

// build state and helpers
typedef struct
{
	TeX_Layout* L;
	TeX_Line* cur_line;
	Node* line_tail;
	int line_count_children;
	int x_cursor;
	int line_asc;
	int line_desc;
	int pending_space;
	const char* pending_space_ptr; // pointer into original input for last spaces
	int pending_space_len; // run length of original spaces
	TexArena* arena;
} BuildState;

static void ensure_line_impl(BuildState* S)
{
	TEX_ASSERT(S != NULL && "ensure_line_impl called with NULL BuildState");
	TEX_ASSERT(S->L != NULL && "ensure_line_impl called with NULL Layout");
	if (!S->cur_line)
	{
		S->cur_line = new_line(S->arena);
		if (!S->cur_line)
		{
			// OOM, do not corrupt the line list
			return;
		}
		append_line(S->L, S->cur_line);
	}
}

static void add_to_line_impl(BuildState* S, Node* n)
{
	TEX_ASSERT(S != NULL && "add_to_line_impl called with NULL BuildState");
	TEX_ASSERT(n != NULL && "add_to_line_impl called with NULL Node");
	ensure_line_impl(S);
	if (!S->cur_line)
	{
		return;
	}
	S->line_asc = TEX_MAX(S->line_asc, n->asc);
	S->line_desc = TEX_MAX(S->line_desc, n->desc);
	n->x = S->x_cursor;
	if (S->line_tail)
	{
		S->line_tail->next = n;
	}
	else
	{
		S->cur_line->first = n;
	}
	S->line_tail = n;
	++S->line_count_children;
	S->x_cursor += n->w;
}

static void finalize_line_impl(BuildState* S)
{
	TEX_ASSERT(S != NULL && "finalize_line_impl called with NULL BuildState");
	if (!S->cur_line)
	{
		return; // nothing to finalize
	}
	if (S->line_tail)
	{
		S->line_tail->next = NULL;
	}
	int h = S->line_asc + S->line_desc + TEX_LINE_LEADING;
	if (h <= 0)
	{
		h = 1;
	}
	S->cur_line->h = h;
	S->cur_line->y = S->L->total_height;
	S->cur_line->child_count = S->line_count_children;
	// clamp total height to prevent overflow
	if (S->L->total_height < TEX_MAX_TOTAL_HEIGHT - h)
	{
		S->L->total_height += h;
	}
	else
	{
		S->L->total_height = TEX_MAX_TOTAL_HEIGHT;
		TEX_SET_ERROR(S->L, TEX_ERR_INPUT, "Document height limit exceeded", S->L->total_height);
#if defined(TEX_DEBUG) && TEX_DEBUG
		S->L->debug_flags |= 0x1u; // height clamped
#endif
	}
	S->cur_line = NULL;
	S->line_tail = NULL;
	S->line_count_children = 0;
	S->x_cursor = 0;
	S->line_asc = 0;
	S->line_desc = 0;
	S->pending_space = 0;
	S->pending_space_ptr = NULL;
	S->pending_space_len = 0;
}

// core formatting
TeX_Layout* tex_format(char* input, int width, TeX_Config* config)
{
	if (!input || width <= 0 || !config)
	{
		return NULL;
	}

	TeX_Layout* L = (TeX_Layout*)calloc(1, sizeof(TeX_Layout));
	if (!L)
	{
		return NULL;
	}
	L->cfg.fg = config->color_fg;
	L->cfg.bg = config->color_bg;
	L->cfg.pack = config->font_pack;
	L->cfg.error_callback = config->error_callback;
	L->cfg.error_userdata = config->error_userdata;
	memset(&L->error, 0, sizeof(L->error));
	L->width = width;
	L->total_height = 0;
#if defined(TEX_DEBUG) && TEX_DEBUG
	L->debug_flags = 0u;
#endif

	tex_metrics_init(L);

	// initialize embedded chunked arena
	arena_init(&L->arena);
	if (L->arena.failed)
	{
		TEX_SET_ERROR(L, TEX_ERR_OOM, "Failed to initialize memory arena", 0);
		free(L);
		return NULL;
	}

	int tok_count = tex_tokenize_top_level(input, NULL, 0, L);
	if (tok_count < 0)
	{
		return L;
	}
	if (tok_count == 0)
	{
		return L; // genuinely empty input
	}
	TeX_Token* toks = (TeX_Token*)malloc((size_t)tok_count * sizeof(TeX_Token));
	if (!toks)
	{
		TEX_SET_ERROR(L, TEX_ERR_OOM, "Failed to allocate token buffer", tok_count);
		return L;
	}
	if (tex_tokenize_top_level(input, toks, tok_count, L) < 0)
	{
		free(toks);
		return L;
	}

	BuildState st;
	st.L = L;
	st.cur_line = NULL;
	st.line_tail = NULL;
	st.line_count_children = 0;
	st.x_cursor = 0;
	st.line_asc = 0;
	st.line_desc = 0;
	st.pending_space = 0;
	st.pending_space_ptr = NULL;
	st.pending_space_len = 0;
	st.arena = &L->arena;

	for (int i = 0; i < tok_count; ++i)
	{
		TeX_Token* t = &toks[i];
		if (t->type == T_EOF)
		{
			break;
		}

		switch (t->type)
		{
		case T_NEWLINE:
			{
				// hard line break
				finalize_line_impl(&st);
			}
			break;

		case T_SPACE:
			{
				// collapse spaces, only set a pending flag
				st.pending_space = 1;
				st.pending_space_ptr = t->start;
				st.pending_space_len = t->len;
			}
			break;

		case T_TEXT:
			{
				// inline coalescing: check if we can extend the existing tail node
				int can_coalesce = 0;
				int include_space = 0;

				if (st.line_tail && st.line_tail->type == N_TEXT)
				{
					const char* tail_end = st.line_tail->data.text.ptr + st.line_tail->data.text.len;

					// check coalescing through a single character space
					if (st.pending_space && st.cur_line && st.pending_space_ptr && st.pending_space_len == 1)
					{
						if (tail_end == st.pending_space_ptr && st.pending_space_ptr + 1 == t->start)
						{
							include_space = 1;
							can_coalesce = 1;
						}
					}

					// check direct contiguity
					else if (!st.pending_space && st.cur_line && tail_end == t->start)
					{
						can_coalesce = 1;
					}
				}

				// wrap check: abort coalescing if it would overflow the line
				if (can_coalesce)
				{
					int space_w = include_space ? tex_metrics_text_width_n(" ", 1, FONTROLE_MAIN) : 0;
					int text_w = tex_metrics_text_width_n(t->start, t->len, FONTROLE_MAIN);

					if (TEX_MATH_ATOMIC_WRAP && (st.x_cursor + space_w + text_w > L->width))
					{
						finalize_line_impl(&st);
						can_coalesce = 0;
						include_space = 0;
					}
				}

				// either coalesce or allocate
				if (can_coalesce)
				{
					// extend the existing tail node without allocation
					int extend_by = t->len + (include_space ? 1 : 0);
					st.line_tail->data.text.len += extend_by;
					tex_measure_node(st.line_tail, FONTROLE_MAIN);
					st.x_cursor = st.line_tail->x + st.line_tail->w;
					st.line_asc = TEX_MAX(st.line_asc, st.line_tail->asc);
					st.line_desc = TEX_MAX(st.line_desc, st.line_tail->desc);
					st.pending_space = 0;
					st.pending_space_ptr = NULL;
					st.pending_space_len = 0;
				}
				else
				{
					// standard allocation path
					Node* n = ln_new_node(L, N_TEXT);
					if (!n)
					{
						st.pending_space = 0;
						st.pending_space_ptr = NULL;
						st.pending_space_len = 0;
						continue;
					}
					n->data.text.ptr = t->start;
					n->data.text.len = t->len;
					tex_measure_node(n, FONTROLE_MAIN);

					// insert collapsed space if pending and line has content
					if (st.pending_space && st.cur_line && st.line_tail)
					{
						Node* sp = ln_new_node(L, N_TEXT);
						if (sp)
						{
							if (st.pending_space_ptr && st.pending_space_len > 0)
							{
								sp->data.text.ptr = st.pending_space_ptr;
								sp->data.text.len = 1;
							}
							else
							{
								sp->data.text.ptr = " ";
								sp->data.text.len = 1;
							}
							tex_measure_node(sp, FONTROLE_MAIN);

							// wrap check, if space + word exceeds width, break before word
							if (TEX_MATH_ATOMIC_WRAP && (st.x_cursor + sp->w + n->w > L->width))
							{
								finalize_line_impl(&st);
							}
							else
							{
								add_to_line_impl(&st, sp);
							}
						}
					}

					// wrap check for the word itself
					if (TEX_MATH_ATOMIC_WRAP && st.cur_line && st.line_tail != NULL && (st.x_cursor + n->w > L->width))
					{
						finalize_line_impl(&st);
					}
					add_to_line_impl(&st, n);
					st.pending_space = 0;
					st.pending_space_ptr = NULL;
					st.pending_space_len = 0;
				}
			}
			break;

		case T_MATH_INLINE:
			{
				// parse math payload and treat as atomic word
				Node* n = tex_parse_math(t->start, t->len, L);
				if (!n && !TEX_HAS_ERROR(L))
				{
					n = ln_new_node(L, N_MATH);
				}
				if (!n)
				{
					// error already set
					break;
				}

				// inline math, clear display flag
				n->flags &= ~MATHF_DISPLAY;
				// measure the math block in main role
				tex_measure_node(n, FONTROLE_MAIN);

				// insert a collapsed space if needed
				if (st.pending_space && st.cur_line && st.line_tail)
				{
					Node* sp = ln_new_node(L, N_TEXT);
					if (sp)
					{
						if (st.pending_space_ptr && st.pending_space_len > 0)
						{
							sp->data.text.ptr = st.pending_space_ptr;
							sp->data.text.len = 1;
						}
						else
						{
							sp->data.text.ptr = " ";
							sp->data.text.len = 1;
						}
						tex_measure_node(sp, FONTROLE_MAIN);
						if (TEX_MATH_ATOMIC_WRAP && (st.x_cursor + sp->w + n->w > L->width))
						{
							finalize_line_impl(&st);
						}
						else
						{
							add_to_line_impl(&st, sp);
						}
					}
					else
					{
						// error already set
					}
				}

				// wrap atomically
				if (TEX_MATH_ATOMIC_WRAP && st.cur_line && st.line_tail != NULL && (st.x_cursor + n->w > L->width))
				{
					finalize_line_impl(&st);
				}
				add_to_line_impl(&st, n);
				st.pending_space = 0;
				st.pending_space_ptr = NULL;
				st.pending_space_len = 0;
			}
			break;

		case T_MATH_DISPLAY:
			{
				finalize_line_impl(&st);
				// create a dedicated centered line with this block
				Node* n = tex_parse_math(t->start, t->len, L);
				if (!n && !TEX_HAS_ERROR(L))
				{
					n = ln_new_node(L, N_MATH);
				}
				if (!n)
				{
					// error already set
					break;
				}

				// display math, set display flag on root
				n->flags |= MATHF_DISPLAY;

				// measure the display math block (with display flags now set)
				tex_measure_node(n, FONTROLE_MAIN);

				// start new line and center
				st.cur_line = new_line(&L->arena);
				if (!st.cur_line)
				{
					TEX_SET_ERROR(L, TEX_ERR_OOM, "Failed to allocate line", 0);
					break;
				}
				append_line(L, st.cur_line);

				// center
				int cx = (L->width - n->w) / 2;
				if (cx < 0)
				{
					cx = 0;
				}
				n->x = cx;
				st.cur_line->first = n;
				st.line_tail = n;
				st.line_count_children = 1;

				st.line_asc = TEX_MAX(st.line_asc, n->asc);
				st.line_desc = TEX_MAX(st.line_desc, n->desc);
				finalize_line_impl(&st);
				st.pending_space = 0;
			}
			break;

		default:
			{
				TEX_ASSERT(0 && "Invalid token type");
			}
			break;
		}
	}

	// finalize any remaining line
	finalize_line_impl(&st);

	// tokens are temporary
	free(toks);

	// post pass, coalesce adjacent text nodes within each finalized line
	coalesce_all_lines(L);

	// build draw time line index
	build_line_index(L);

	// shouldnt be needed?
	if (L->arena.failed)
	{
		TEX_SET_ERROR(L, TEX_ERR_OOM, "Arena allocation failed during layout", 0);
	}
	return L;
}

int tex_get_total_height(TeX_Layout* layout)
{
	if (!layout)
	{
		return 0;
	}
	return layout->total_height;
}

void tex_free(TeX_Layout* layout)
{
	if (!layout)
	{
		return;
	}

	arena_free_all(&layout->arena);
	free(layout);
}

TeX_Error tex_get_last_error(TeX_Layout* layout)
{
	if (!layout)
	{
		return TEX_ERR_INPUT;
	}
	return (TeX_Error)layout->error.code;
}

const char* tex_get_error_message(TeX_Layout* layout)
{
	if (!layout)
	{
		return "Invalid layout";
	}
	return layout->error.msg ? layout->error.msg : "";
}

int tex_get_error_value(TeX_Layout* layout)
{
	if (!layout)
	{
		return 0;
	}
	return layout->error.val;
}
