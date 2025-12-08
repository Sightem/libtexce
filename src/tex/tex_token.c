#include "tex_token.h"
#include <string.h>
#include "tex_arena.h"
#include "tex_internal.h"
#include "tex_util.h"

static const char* find_math_end(const char* start, const char* end, int is_display)
{
	const char* p = start;
	if (!p)
		return NULL;

	while (p < end && *p)
	{
		if (*p == '\\')
		{
			if (p + 1 < end && *(p + 1))
				p += 2;
			else
				break;
			continue;
		}
		if (*p == '$')
		{
			if (is_display)
			{
				if (p + 1 < end && *(p + 1) == '$')
					return p;
			}
			else
			{
				return p;
			}
		}
		++p;
	}
	return NULL;
}

void tex_stream_init(TeX_Stream* s, const char* input, int len)
{
	TEX_ASSERT(s != NULL && "tex_stream_init called with NULL stream");
	if (!s)
		return;

	s->cursor = input ? input : "";
	if (len < 0 && input)
	{
		s->end = input + strlen(input);
	}
	else if (input)
	{
		s->end = input + len;
	}
	else
	{
		s->end = s->cursor;
	}
}

// fill token with unescaped text
// returns 0 on success, -1 on error
static int fill_text_token(TeX_Token* out, TokenType type, const char* s, int raw_len, TexArena* arena,
                           TeX_Layout* layout)
{
	int unescaped_len = tex_util_unescaped_len(s, raw_len);

	if (unescaped_len == raw_len || !arena)
	{
		out->type = type;
		out->start = s;
		out->len = unescaped_len;
		out->aux = 0;
		return 0;
	}

	char* buf = (char*)arena_alloc(arena, (size_t)unescaped_len + 1, 1);
	if (!buf)
	{
		if (layout)
		{
			TEX_SET_ERROR(layout, TEX_ERR_OOM, "Failed to allocate unescaped token", unescaped_len);
		}
		return -1;
	}

	tex_util_copy_unescaped(buf, s, raw_len);
	out->type = type;
	out->start = buf;
	out->len = unescaped_len;
	out->aux = 0;
	return 0;
}

int tex_stream_next(TeX_Stream* s, TeX_Token* out, TexArena* arena, TeX_Layout* layout)
{
	TEX_ASSERT(s != NULL && "tex_stream_next called with NULL stream");
	TEX_ASSERT(out != NULL && "tex_stream_next called with NULL out token");

	if (!s || !out)
		return 0;

	if (s->cursor >= s->end || !*s->cursor)
	{
		out->type = T_EOF;
		out->start = s->cursor;
		out->len = 0;
		out->aux = 0;
		return 0;
	}

	const char* p = s->cursor;

	// Newline
	if (*p == '\n')
	{
		out->type = T_NEWLINE;
		out->start = p;
		out->len = 1;
		out->aux = 0;
		s->cursor = p + 1;
		return 1;
	}

	// Space run
	if (*p == ' ')
	{
		const char* start = p;
		while (p < s->end && *p == ' ')
			++p;
		out->type = T_SPACE;
		out->start = start;
		out->len = (int)(p - start);
		out->aux = 0;
		s->cursor = p;
		return 1;
	}

	// Math mode
	if (*p == '$')
	{
		int is_display = 0;
		const char* after_open = p + 1;
		if (after_open < s->end && *after_open == '$')
		{
			is_display = 1;
			after_open = p + 2;
		}

		const char* close = find_math_end(after_open, s->end, is_display);
		if (close)
		{
			int raw_len = (int)(close - after_open);
			TokenType tt = is_display ? T_MATH_DISPLAY : T_MATH_INLINE;
			// math content passes through verbatim, math parser handles its own escapes
			out->type = tt;
			out->start = after_open;
			out->len = raw_len;
			out->aux = 0;
			s->cursor = close + (is_display ? 2 : 1);
			return 1;
		}
		else
		{
			// unclosed math: treat '$' as starting a text run
			const char* start = p;
			++p;
			while (p < s->end && *p && *p != ' ' && *p != '\n' && *p != '$')
			{
				if (*p == '\\' && p + 1 < s->end && *(p + 1) && tex_util_is_escape_char(*(p + 1)))
					p += 2;
				else
					++p;
			}
			int raw_len = (int)(p - start);
			if (fill_text_token(out, T_TEXT, start, raw_len, arena, layout) < 0)
			{
				return 0;
			}
			s->cursor = p;
			return 1;
		}
	}

	// plain text run
	{
		const char* start = p;
		while (p < s->end && *p && *p != ' ' && *p != '\n' && *p != '$')
		{
			if (*p == '\\' && p + 1 < s->end && *(p + 1) && tex_util_is_escape_char(*(p + 1)))
				p += 2;
			else
				++p;
		}
		int raw_len = (int)(p - start);
		if (fill_text_token(out, T_TEXT, start, raw_len, arena, layout) < 0)
		{
			return 0;
		}
		s->cursor = p;
		return 1;
	}
}
