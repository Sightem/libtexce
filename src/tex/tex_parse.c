#include "tex_parse.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "tex_symbols.h"
#include "tex_util.h"

// ------------------
// helpers
// ------------------


typedef enum
{
	M_EOF = 0,
	M_CHAR,
	M_CMD,
	M_LBRACE,
	M_RBRACE,
	M_CARET,
	M_UNDER,
	M_LBRACKET, // '['
	M_RBRACKET, // ']'
} MTokenKind;

typedef struct
{
	MTokenKind kind;
	const char* start;
	int len;
} MToken;

typedef struct
{
	const char* cur;
	const char* end;
} MLex;


typedef struct
{
	MLex lx;
	int depth;
	TeX_Layout* L; // provides both arena access and error reporting
} Parser;


static Node* new_node(Parser* p, NodeType t)
{
	if (!p || !p->L)
		return NULL;
	if (TEX_HAS_ERROR(p->L))
		return NULL;

	Node* n = (Node*)arena_alloc(&p->L->arena, sizeof(Node), sizeof(void*));
	if (!n)
	{
		TEX_SET_ERROR(p->L, TEX_ERR_OOM, "Failed to allocate parse node", 0);
		return NULL;
	}
	memset(n, 0, sizeof(Node));
	n->type = (uint8_t)t;
	return n;
}

static Node* make_text(Parser* p, const char* s, size_t len)
{
	Node* n = new_node(p, N_TEXT);
	if (!n)
		return NULL;
	n->data.text.ptr = s;
	n->data.text.len = (int)len;
	return n;
}

static Node* make_glyph(Parser* p, uint16_t code)
{
	Node* n = new_node(p, N_GLYPH);
	if (!n)
		return NULL;
	n->data.glyph = code;
	return n;
}

static Node* make_multiop(Parser* p, uint8_t count, uint8_t op_type)
{
	Node* n = new_node(p, N_MULTIOP);
	if (!n)
		return NULL;
	n->data.multiop.count = count;
	n->data.multiop.op_type = op_type;
	return n;
}
static void ml_init(MLex* lx, const char* s, int len)
{
	TEX_ASSERT(len >= 0);
	if (len < 0)
		len = 0;
	lx->cur = s;
	lx->end = s + len;
}

static int ml_at_end(MLex* lx) { return lx->cur >= lx->end; }

static MToken ml_next(MLex* lx)
{
	while (!ml_at_end(lx) && *lx->cur == ' ')
		++lx->cur; // ignore spaces
	if (ml_at_end(lx))
	{
		MToken t = { M_EOF, lx->cur, 0 };
		return t;
	}
	char c = *lx->cur;
	if (c == '{')
	{
		++lx->cur;
		MToken t = { M_LBRACE, lx->cur - 1, 1 };
		return t;
	}
	if (c == '}')
	{
		++lx->cur;
		MToken t = { M_RBRACE, lx->cur - 1, 1 };
		return t;
	}
	if (c == '^')
	{
		++lx->cur;
		MToken t = { M_CARET, lx->cur - 1, 1 };
		return t;
	}
	if (c == '_')
	{
		++lx->cur;
		MToken t = { M_UNDER, lx->cur - 1, 1 };
		return t;
	}
	if (c == '[')
	{
		++lx->cur;
		MToken t = { M_LBRACKET, lx->cur - 1, 1 };
		return t;
	}
	if (c == ']')
	{
		++lx->cur;
		MToken t = { M_RBRACKET, lx->cur - 1, 1 };
		return t;
	}
	if (c == '\\')
	{
		const char* s = ++lx->cur;
		while (!ml_at_end(lx) && isalpha((unsigned char)*lx->cur))
			++lx->cur;
		int len = (int)(lx->cur - s);
		if (len <= 0 && !ml_at_end(lx))
		{
			// single-char command like \^
			s = lx->cur;
			len = 1;
			++lx->cur;
		}
		MToken t = { M_CMD, s, len };
		return t;
	}
	// default: character
	++lx->cur;
	MToken t = { M_CHAR, lx->cur - 1, 1 };
	return t;
}

// ------------------
// recursive descent
// ------------------

static MToken ml_peek(MLex* lx)
{
	MLex tmp = *lx;
	return ml_next(&tmp);
}

static Node* parse_math_list(Parser* p);
static Node* parse_atom(Parser* p);
static Node* parse_command(Parser* p, const char* name, int len);

static Node* wrap_group(Parser* p, Node* head)
{
	Node* n = new_node(p, N_MATH);
	if (!n)
		return NULL;
	n->child = head;
	return n;
}


static inline int is_script_marker(MTokenKind kind) { return kind == M_CARET || kind == M_UNDER; }

// append node to a linked list
static inline void append_node(Node** head, Node** tail, Node* n)
{
	if (!n)
		return;
	if (!*head)
	{
		*head = *tail = n;
	}
	else
	{
		(*tail)->next = n;
		*tail = n;
	}
}

// flush pending character run to the node list
// if script_follows is true: emit all but last as N_TEXT, return last char via out_script_base
// otherwise: emit entire run as one node (N_TEXT if len>1, N_GLYPH if len==1)
static void flush_char_run(Parser* p, const char* run_start, int run_len, int script_follows, Node** head, Node** tail,
                           Node** out_script_base)
{
	if (out_script_base)
		*out_script_base = NULL;
	if (run_len <= 0)
		return;

	if (script_follows)
	{
		// emit all but last character as N_TEXT (if more than one char)
		if (run_len > 1)
		{
			Node* txt = make_text(p, run_start, (size_t)(run_len - 1));
			append_node(head, tail, txt);
		}
		// last character becomes the script base (not appended yet)
		if (out_script_base)
		{
			*out_script_base = make_glyph(p, (uint8_t)run_start[run_len - 1]);
		}
	}
	else
	{
		Node* n;
		if (run_len == 1)
		{
			n = make_glyph(p, (uint8_t)*run_start);
		}
		else
		{
			n = make_text(p, run_start, (size_t)run_len);
		}
		append_node(head, tail, n);
	}
}

static Node* parse_script_arg(Parser* p);

static Node* attach_scripts(Parser* p, Node* base)
{
	if (!base)
		return NULL;
	Node* sub = NULL;
	Node* sup = NULL;
	int again = 1;
	while (again)
	{
		again = 0;
		MToken k = ml_peek(&p->lx);
		if (k.kind == M_CARET)
		{
			(void)ml_next(&p->lx);
			sup = sup ? sup : parse_script_arg(p);
			if (!sup && !TEX_HAS_ERROR(p->L))
			{
				TEX_SET_ERROR(p->L, TEX_ERR_PARSE, "Missing argument for superscript/subscript", 0);
				return NULL;
			}
			again = 1;
		}
		else if (k.kind == M_UNDER)
		{
			(void)ml_next(&p->lx);
			sub = sub ? sub : parse_script_arg(p);
			if (!sub && !TEX_HAS_ERROR(p->L))
			{
				TEX_SET_ERROR(p->L, TEX_ERR_PARSE, "Missing argument for superscript/subscript", 0);
				return NULL;
			}
			again = 1;
		}
	}
	if (sub || sup)
	{
		Node* s = new_node(p, N_SCRIPT);
		if (!s)
			return NULL;
		s->data.script.base = base;
		s->data.script.sub = sub;
		s->data.script.sup = sup;
		return s;
	}
	return base;
}


static Node* parse_group(Parser* p)
{
	// assumes next token is '{'
	(void)ml_next(&p->lx);

	// depth guard
	p->depth++;
	if (p->depth > TEX_PARSE_MAX_DEPTH)
	{
		TEX_SET_ERROR(p->L, TEX_ERR_DEPTH, "Maximum nesting depth exceeded in group", p->depth);
		p->depth--;
		return NULL;
	}

	Node* head = NULL;
	Node* tail = NULL;

	// pending run of contiguous characters
	const char* run_start = NULL;
	int run_len = 0;

	while (1)
	{
		MToken pk = ml_peek(&p->lx);
		if (pk.kind == M_RBRACE || pk.kind == M_EOF)
		{
			if (pk.kind == M_RBRACE)
				(void)ml_next(&p->lx); // consume '}'
			break;
		}
		if (TEX_HAS_ERROR(p->L))
			break;

		if (pk.kind == M_CHAR)
		{
			// accumulate character
			if (run_len == 0)
			{
				run_start = pk.start;
				run_len = 1;
			}
			else if (pk.start == run_start + run_len)
			{
				run_len++;
			}
			else
			{
				flush_char_run(p, run_start, run_len, 0, &head, &tail, NULL);
				run_start = pk.start;
				run_len = 1;
			}
			ml_next(&p->lx);

			MToken next = ml_peek(&p->lx);
			if (is_script_marker(next.kind))
			{
				Node* base = NULL;
				flush_char_run(p, run_start, run_len, 1, &head, &tail, &base);
				run_start = NULL;
				run_len = 0;

				if (base)
				{
					base = attach_scripts(p, base);
					append_node(&head, &tail, base);
				}
			}
		}
		else if (is_script_marker(pk.kind))
		{
			flush_char_run(p, run_start, run_len, 0, &head, &tail, NULL);
			run_start = NULL;
			run_len = 0;

			ml_next(&p->lx);
			Node* n = make_glyph(p, (pk.kind == M_CARET) ? '^' : '_');
			append_node(&head, &tail, n);
		}
		else
		{
			flush_char_run(p, run_start, run_len, 0, &head, &tail, NULL);
			run_start = NULL;
			run_len = 0;

			Node* it = parse_atom(p);
			if (TEX_HAS_ERROR(p->L))
				break;
			if (!it)
				break;

			MToken next = ml_peek(&p->lx);
			if (is_script_marker(next.kind))
			{
				it = attach_scripts(p, it);
			}

			append_node(&head, &tail, it);
		}
	}

	flush_char_run(p, run_start, run_len, 0, &head, &tail, NULL);

	p->depth--;
	return wrap_group(p, head);
}

// parse an optional bracketed argument [ ... ]; returns NULL if not present
static Node* parse_optional_bracket_arg(Parser* p)
{
	MToken pk = ml_peek(&p->lx);
	if (pk.kind != M_LBRACKET)
		return NULL;

	// consume '['
	(void)ml_next(&p->lx);
	// TODO: should we assert here?

	// depth guard
	p->depth++;
	if (p->depth > TEX_PARSE_MAX_DEPTH)
	{
		TEX_SET_ERROR(p->L, TEX_ERR_DEPTH, "Maximum nesting depth exceeded in bracket arg", p->depth);
		p->depth--;
		return NULL;
	}

	Node* head = NULL;
	Node* tail = NULL;

	// pending character run
	const char* run_start = NULL;
	int run_len = 0;

	while (1)
	{
		MToken pk2 = ml_peek(&p->lx);
		if (pk2.kind == M_RBRACKET)
		{
			(void)ml_next(&p->lx); // consume ']'
			break;
		}
		if (pk2.kind == M_EOF)
		{
			TEX_SET_ERROR(p->L, TEX_ERR_PARSE, "Unclosed '[' in optional argument", 0);
			p->depth--;
			return NULL;
		}
		if (TEX_HAS_ERROR(p->L))
			break;

		if (pk2.kind == M_CHAR)
		{
			if (run_len == 0)
			{
				run_start = pk2.start;
				run_len = 1;
			}
			else if (pk2.start == run_start + run_len)
			{
				run_len++;
			}
			else
			{
				flush_char_run(p, run_start, run_len, 0, &head, &tail, NULL);
				run_start = pk2.start;
				run_len = 1;
			}
			ml_next(&p->lx);

			MToken next = ml_peek(&p->lx);
			if (is_script_marker(next.kind))
			{
				Node* base = NULL;
				flush_char_run(p, run_start, run_len, 1, &head, &tail, &base);
				run_start = NULL;
				run_len = 0;

				if (base)
				{
					base = attach_scripts(p, base);
					append_node(&head, &tail, base);
				}
			}
		}
		else if (is_script_marker(pk2.kind))
		{
			flush_char_run(p, run_start, run_len, 0, &head, &tail, NULL);
			run_start = NULL;
			run_len = 0;

			ml_next(&p->lx);
			Node* n = make_glyph(p, (pk2.kind == M_CARET) ? '^' : '_');
			append_node(&head, &tail, n);
		}
		else
		{
			flush_char_run(p, run_start, run_len, 0, &head, &tail, NULL);
			run_start = NULL;
			run_len = 0;

			Node* item = parse_atom(p);
			if (TEX_HAS_ERROR(p->L))
			{
				p->depth--;
				return NULL;
			}
			if (!item)
				break;

			append_node(&head, &tail, item);
		}
	}

	flush_char_run(p, run_start, run_len, 0, &head, &tail, NULL);

	p->depth--;

	// treat empty [] as no index
	if (!head)
		return NULL;

	return wrap_group(p, head);
}

// parse one atom without absorbing trailing ^/_ scripts
static Node* parse_atom_noscript(Parser* p)
{
	MToken t = ml_peek(&p->lx);
	if (t.kind == M_LBRACE)
		return parse_group(p);
	if (t.kind == M_CMD)
	{
		(void)ml_next(&p->lx);
		return parse_command(p, t.start, t.len);
	}
	if (t.kind == M_CHAR)
	{
		(void)ml_next(&p->lx);
		return make_glyph(p, (uint8_t)*t.start);
	}
	if (t.kind == M_LBRACKET || t.kind == M_RBRACKET)
	{
		(void)ml_next(&p->lx);
		// treat [ and ] as standard ASCII glyphs here
		return make_glyph(p, (t.kind == M_LBRACKET) ? '[' : ']');
	}
	if (t.kind == M_CARET || t.kind == M_UNDER)
	{
		(void)ml_next(&p->lx);
		return make_glyph(p, (t.kind == M_CARET) ? '^' : '_');
	}
	if (t.kind == M_RBRACE || t.kind == M_EOF)
		return NULL;
	(void)ml_next(&p->lx);
	TEX_SET_ERROR(p->L, TEX_ERR_PARSE, "Unexpected token in math expression", 0);
	return NULL;
}

static Node* parse_script_arg(Parser* p)
{
	MToken pk = ml_peek(&p->lx);
	if (pk.kind == M_LBRACE)
		return parse_group(p);
	return parse_atom_noscript(p);
}


static const struct
{
	const char* str;
	int len;
} g_func_text[] = {
	{ NULL, 0 }, // 0 (unused)
	{ "sin", 3 }, // SYMC_FUNC_SIN
	{ "cos", 3 }, // SYMC_FUNC_COS
	{ "tan", 3 }, // SYMC_FUNC_TAN
	{ "ln", 2 }, // SYMC_FUNC_LN
	{ "lim", 3 }, // SYMC_FUNC_LIM
	{ "log", 3 }, // SYMC_FUNC_LOG
	{ "exp", 3 }, // SYMC_FUNC_EXP
	{ "min", 3 }, // SYMC_FUNC_MIN
	{ "max", 3 }, // SYMC_FUNC_MAX
	{ "sup", 3 }, // SYMC_FUNC_SUP
	{ "inf", 3 }, // SYMC_FUNC_INF
	{ "det", 3 }, // SYMC_FUNC_DET
	{ "gcd", 3 }, // SYMC_FUNC_GCD
	{ "deg", 3 }, // SYMC_FUNC_DEG
	{ "dim", 3 }, // SYMC_FUNC_DIM
	{ "sec", 3 }, // SYMC_FUNC_SEC
	{ "csc", 3 }, // SYMC_FUNC_CSC
	{ "cot", 3 }, // SYMC_FUNC_COT
	{ "arcsin", 6 }, // SYMC_FUNC_ARCSIN
	{ "arccos", 6 }, // SYMC_FUNC_ARCCOS
	{ "arctan", 6 }, // SYMC_FUNC_ARCTAN
	{ "sinh", 4 }, // SYMC_FUNC_SINH
	{ "cosh", 4 }, // SYMC_FUNC_COSH
	{ "tanh", 4 }, // SYMC_FUNC_TANH
	{ "arg", 3 }, // SYMC_FUNC_ARG
	{ "ker", 3 }, // SYMC_FUNC_KER
	{ "Pr", 2 }, // SYMC_FUNC_PR
	{ "hom", 3 }, // SYMC_FUNC_HOM
	{ "lg", 2 }, // SYMC_FUNC_LG
	{ "coth", 4 }, // SYMC_FUNC_COTH
};
static const int func_names_count = (int)(sizeof(g_func_text) / sizeof(g_func_text[0]));


static Node* parse_text_arg(Parser* p)
{
	// expect opening brace
	MToken t = ml_peek(&p->lx);
	if (t.kind != M_LBRACE)
	{
		TEX_SET_ERROR(p->L, TEX_ERR_PARSE, "expected '{' after \\text", 0);
		return NULL;
	}

	// consume '{'
	ml_next(&p->lx);

	// scan raw content until '}' handling escapes
	const char* start = p->lx.cur;
	const char* cur = start;
	int needs_unescape = 0;

	while (cur < p->lx.end)
	{
		if (*cur == '}')
		{
			break; // found closing brace
		}
		if (*cur == '\\')
		{
			needs_unescape = 1;
			// skip next char (escape sequence)
			if (cur + 1 < p->lx.end)
			{
				cur += 2;
				continue;
			}
		}
		cur++;
	}

	if (cur >= p->lx.end)
	{
		TEX_SET_ERROR(p->L, TEX_ERR_PARSE, "Unclosed \\text argument", 0);
		return NULL;
	}

	// length of the segment inside { ... }
	int raw_len = (int)(cur - start);

	// create N_TEXT node
	Node* n = new_node(p, N_TEXT);
	if (!n)
		return NULL;

	if (needs_unescape)
	{
		// allocate persistent storage in Arena
		int ulen = tex_util_unescaped_len(start, raw_len);
		char* buf = (char*)arena_alloc(&p->L->arena, (size_t)ulen + 1U, 1);
		if (!buf)
		{
			TEX_SET_ERROR(p->L, TEX_ERR_OOM, "OOM parsing \\text", 0);
			return NULL;
		}
		tex_util_copy_unescaped(buf, start, raw_len);
		n->data.text.ptr = buf;
		n->data.text.len = ulen;
	}
	else
	{
		// point directly into input string
		n->data.text.ptr = start;
		n->data.text.len = raw_len;
	}

	// manually advance lexer past the processed text and the closing '}'
	p->lx.cur = cur + 1;

	return n;
}

static Node* parse_command(Parser* p, const char* name, int len)
{
	SymbolDesc d;
	memset(&d, 0, sizeof(d));
	int found = texsym_find(name, (size_t)len, &d);

	if (!found)
	{
		return make_text(p, name, (size_t)len);
	}
	switch (d.kind)
	{
	case SYM_GLYPH:
		return make_glyph(p, d.code);
	case SYM_SPACE:
		{
			Node* sp = new_node(p, N_SPACE);
			if (!sp)
				return NULL;
			int w = 0;
			if (d.code == SYMC_THINSPACE)
				w = 2; // \,
			else if (d.code == SYMC_MEDSPACE)
				w = 3; // \:
			else if (d.code == SYMC_THICKSPACE)
				w = 4; // \;
			else if (d.code == SYMC_NEGSPACE)
				w = -1; // \!
			else if (d.code == SYMC_QUAD)
			{
				sp->data.space.em_mul = 1;
				w = 0;
			}
			else if (d.code == SYMC_QQUAD)
			{
				sp->data.space.em_mul = 2;
				w = 0;
			}
			sp->data.space.width = w;
			return sp;
		}
	case SYM_ACCENT:
		{
			Node* base = NULL;
			MToken pk = ml_peek(&p->lx);
			if (pk.kind == M_LBRACE)
				base = parse_group(p);
			else
				base = parse_atom(p);
			Node* ov = new_node(p, N_OVERLAY);
			if (!ov)
				return NULL;
			ov->data.overlay.base = base;
			// Map code -> AccentType
			uint8_t at = 0;
			if (d.code == SYMC_ACC_VEC)
				at = ACC_VEC;
			else if (d.code == SYMC_ACC_HAT)
				at = ACC_HAT;
			else if (d.code == SYMC_ACC_BAR)
				at = ACC_BAR;
			else if (d.code == SYMC_ACC_DOT)
				at = ACC_DOT;
			else if (d.code == SYMC_ACC_DDOT)
				at = ACC_DDOT;
			else if (d.code == SYMC_ACC_OVERLINE)
				at = ACC_OVERLINE;
			else if (d.code == SYMC_ACC_UNDERLINE)
				at = ACC_UNDERLINE;
			ov->data.overlay.type = at;
			return ov;
		}
	case SYM_STRUCT:
		{
			if (d.code == SYMC_TEXT)
			{
				return parse_text_arg(p);
			}
			if (d.code == SYMC_FRAC)
			{
				// \frac{num}{den}
				Node* num = NULL;
				Node* den = NULL;
				if (ml_peek(&p->lx).kind == M_LBRACE)
					num = parse_group(p);
				else
					num = parse_atom(p);
				if (ml_peek(&p->lx).kind == M_LBRACE)
					den = parse_group(p);
				else
					den = parse_atom(p);
				Node* f = new_node(p, N_FRAC);
				if (!f)
					return NULL;
				f->data.frac.num = num;
				f->data.frac.den = den;
				return f;
			}
			else if (d.code == SYMC_SQRT)
			{
				Node* index = parse_optional_bracket_arg(p);
				if (TEX_HAS_ERROR(p->L))
					return NULL;
				Node* rad = NULL;
				MToken pk2 = ml_peek(&p->lx);
				if (pk2.kind == M_LBRACE)
				{
					rad = parse_group(p);
				}
				else if (pk2.kind == M_EOF || pk2.kind == M_RBRACE)
				{
					TEX_SET_ERROR(p->L, TEX_ERR_PARSE, "Missing argument for \\sqrt", 0);
					return NULL;
				}
				else
				{
					rad = parse_atom(p);
				}
				if (TEX_HAS_ERROR(p->L))
					return NULL;
				Node* s = new_node(p, N_SQRT);
				if (!s)
					return NULL;
				s->data.sqrt.rad = rad;
				s->data.sqrt.index = index; // NULL if not provided
				return s;
			}
			else if (d.code == SYMC_OVERBRACE || d.code == SYMC_UNDERBRACE)
			{
				Node* content = NULL;
				MToken pk = ml_peek(&p->lx);
				if (pk.kind == M_LBRACE)
					content = parse_group(p);
				else
					content = parse_atom(p);

				Node* n = new_node(p, N_SPANDECO);
				if (!n)
					return NULL;
				n->data.spandeco.content = content;
				n->data.spandeco.label = NULL;
				n->data.spandeco.deco_type = (d.code == SYMC_OVERBRACE) ? DECO_OVERBRACE : DECO_UNDERBRACE;

				// ^ for overbrace, _ for underbrace
				MToken k = ml_peek(&p->lx);
				if ((d.code == SYMC_OVERBRACE && k.kind == M_CARET) || (d.code == SYMC_UNDERBRACE && k.kind == M_UNDER))
				{
					(void)ml_next(&p->lx);
					n->data.spandeco.label = parse_script_arg(p);
				}
				return n;
			}
			break;
		}
	case SYM_FUNC:
		{
			// functions render as upright text, lim is special with under-limit
			if (d.code == SYMC_FUNC_LIM)
			{
				Node* lim = new_node(p, N_FUNC_LIM);
				if (!lim)
					return NULL;
				// optional under-limit
				MToken pk = ml_peek(&p->lx);
				if (pk.kind == M_UNDER)
				{
					(void)ml_next(&p->lx);
					Node* arg = parse_script_arg(p);
					lim->data.func_lim.limit = arg;
				}
				return lim;
			}

			if (d.code > 0 && d.code < func_names_count)
			{
				TEX_ASSERT(g_func_text[d.code].str != NULL && "Missing string mapping for SYM_FUNC");

				if (g_func_text[d.code].str)
				{
					Node* n = new_node(p, N_TEXT);
					if (!n)
						return NULL;
					n->data.text.ptr = g_func_text[d.code].str;
					n->data.text.len = g_func_text[d.code].len;
					return n;
				}
			}
			break;
		}
	case SYM_MULTIOP:
		{
			uint8_t count = 2; // default for \iint
			uint8_t op_type = MULTIOP_INT;

			if (d.code == SYMC_MULTIINT_2)
				count = 2;
			else if (d.code == SYMC_MULTIINT_3)
				count = 3;
			else if (d.code == SYMC_MULTIINT_4)
				count = 4;
			else if (d.code == SYMC_MULTI_OINT_1)
			{
				count = 1;
				op_type = MULTIOP_OINT;
			}
			else if (d.code == SYMC_MULTI_OINT_2)
			{
				count = 2;
				op_type = MULTIOP_OINT;
			}
			else if (d.code == SYMC_MULTI_OINT_3)
			{
				count = 3;
				op_type = MULTIOP_OINT;
			}

			return make_multiop(p, count, op_type);
		}
	default:
		break;
	}
	// fallback to literal text
	return make_text(p, name, (size_t)len);
}

static Node* parse_atom(Parser* p)
{
	MToken t = ml_peek(&p->lx);
	if (t.kind == M_LBRACE)
	{
		return attach_scripts(p, parse_group(p));
	}
	if (t.kind == M_CMD)
	{
		(void)ml_next(&p->lx);
		return attach_scripts(p, parse_command(p, t.start, t.len));
	}
	if (t.kind == M_CHAR)
	{
		(void)ml_next(&p->lx);
		Node* n = make_glyph(p, (uint8_t)*t.start);
		return attach_scripts(p, n);
	}
	if (t.kind == M_LBRACKET || t.kind == M_RBRACKET)
	{
		(void)ml_next(&p->lx);
		Node* n = make_glyph(p, (t.kind == M_LBRACKET) ? '[' : ']');
		return attach_scripts(p, n);
	}
	if (t.kind == M_CARET || t.kind == M_UNDER)
	{
		(void)ml_next(&p->lx);
		Node* n = make_glyph(p, (t.kind == M_CARET) ? '^' : '_');
		return n;
	}
	if (t.kind == M_RBRACE)
	{
		// caller handles '}'
		return NULL;
	}
	if (t.kind == M_EOF)
		return NULL;

	(void)ml_next(&p->lx);
	TEX_SET_ERROR(p->L, TEX_ERR_PARSE, "Unexpected token in math expression", 0);
	return NULL;
}

static Node* parse_math_list(Parser* p)
{
	Node* head = NULL;
	Node* tail = NULL;

	// pending run of contiguous ASCII characters
	const char* run_start = NULL;
	int run_len = 0;

	while (1)
	{
		MToken pk = ml_peek(&p->lx);
		if (pk.kind == M_EOF || pk.kind == M_RBRACE)
			break;
		if (TEX_HAS_ERROR(p->L))
			break;

		if (pk.kind == M_CHAR)
		{
			// accumulate character into pending run
			if (run_len == 0)
			{
				run_start = pk.start;
				run_len = 1;
			}
			else if (pk.start == run_start + run_len)
			{
				// contiguous in source buffer
				run_len++;
			}
			else
			{
				// noncontiguous: flush current run, start new one
				flush_char_run(p, run_start, run_len, 0, &head, &tail, NULL);
				run_start = pk.start;
				run_len = 1;
			}
			ml_next(&p->lx); // consume the character token

			// check if a script marker follows this character
			MToken next = ml_peek(&p->lx);
			if (is_script_marker(next.kind))
			{
				// script follows: flush run with last char as script base
				Node* base = NULL;
				flush_char_run(p, run_start, run_len, 1, &head, &tail, &base);
				run_start = NULL;
				run_len = 0;

				// attach scripts to the base glyph
				if (base)
				{
					base = attach_scripts(p, base);
					append_node(&head, &tail, base);
				}
			}
			// else: continue accumulating
		}
		else if (is_script_marker(pk.kind))
		{
			// script marker without preceding character (standalone ^ or _)
			flush_char_run(p, run_start, run_len, 0, &head, &tail, NULL);
			run_start = NULL;
			run_len = 0;

			ml_next(&p->lx);
			Node* n = make_glyph(p, (pk.kind == M_CARET) ? '^' : '_');
			append_node(&head, &tail, n);
		}
		else
		{
			// other token
			flush_char_run(p, run_start, run_len, 0, &head, &tail, NULL);
			run_start = NULL;
			run_len = 0;

			Node* n = parse_atom(p);
			if (TEX_HAS_ERROR(p->L) || !n)
				break;
			append_node(&head, &tail, n);
		}
	}

	// flush any remaining accumulated characters
	flush_char_run(p, run_start, run_len, 0, &head, &tail, NULL);

	return head;
}

Node* tex_parse_math(const char* input, int len, TeX_Layout* layout)
{
	if (!layout)
		return NULL;

	if (!input)
	{
		TEX_SET_ERROR(layout, TEX_ERR_INPUT, "NULL input to math parser", 0);
		return NULL;
	}
	if (len < 0)
		len = (int)strlen(input);

	Parser p;
	ml_init(&p.lx, input, len);
	p.depth = 0;
	p.L = layout;

	Node* seq = parse_math_list(&p);
	if (TEX_HAS_ERROR(layout))
	{
		return NULL;
	}

	Node* root = new_node(&p, N_MATH);
	if (!root)
		return NULL; // error already set by new_node

	root->child = seq;
	return root;
}
