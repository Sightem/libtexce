#include "tex_token.h"
#include <string.h>
#include "tex_internal.h"
#include "tex_util.h"

static const char* find_math_end(const char* start, int is_display)
{
	const char* p = start;
	if (!p)
		return NULL;

	while (*p)
	{
		if (*p == '\\')
		{
			if (*(p + 1))
				p += 2;
			else
				break;
			continue;
		}
		if (*p == '$')
		{
			if (is_display)
			{
				if (*(p + 1) == '$')
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

// push a token into the output array
// returns 0 on success, -1 on overflow (error set on layout)
static int push_token(TeX_Token* out, int max, int* count, TeX_Layout* L, TokenType type, const char* start, int len)
{
	int idx = *count;

	if (out && idx >= max)
	{
		TEX_SET_ERROR(L, TEX_ERR_INPUT, "Token buffer overflow", idx);
		return -1;
	}

	if (out && idx < max)
	{
		out[idx].type = type;
		out[idx].start = start;
		out[idx].len = len;
		out[idx].aux = 0;
	}

	*count = idx + 1;
	return 0;
}

// emit a text like token, handling escape sequences
// returns 0 on success, -1 on error (error set on layout)
static int emit_text_token(TeX_Token* out, int max, int* count, TeX_Layout* L, TokenType type, const char* s,
                           int raw_len, int counting_only)
{
	int need = tex_util_unescaped_len(s, raw_len);

	// pass 1 (counting) or no escapes needed, direct push
	if (counting_only || need == raw_len)
	{
		return push_token(out, max, count, L, type, s, need);
	}

	// need to allocate and unescape
	char* buf = (char*)arena_alloc(&L->arena, (size_t)need + 1, 1);
	if (!buf)
	{
		TEX_SET_ERROR(L, TEX_ERR_OOM, "Failed to allocate unescaped token", need);
		return -1;
	}

	tex_util_copy_unescaped(buf, s, raw_len);
	return push_token(out, max, count, L, type, buf, need);
}

int tex_tokenize_top_level(const char* input, TeX_Token* out_tokens, int max_tokens, TeX_Layout* layout)
{
	TEX_ASSERT(input != NULL && "tex_tokenize_top_level called with NULL input");

	if (!input)
	{
		if (layout)
			TEX_SET_ERROR(layout, TEX_ERR_INPUT, "NULL input to tokenizer", 0);
		return -1;
	}

	if (!layout)
	{
		return -1; // cannot proceed without layout for error reporting / arena
	}

	int total = 0;
	const char* p = input;
	const int counting_only = (out_tokens == NULL);

	while (*p)
	{
		// check for prior error (from push_token or emit_text_token)
		if (TEX_HAS_ERROR(layout))
		{
			return -1;
		}

		if (*p == '\n')
		{
			if (push_token(out_tokens, max_tokens, &total, layout, T_NEWLINE, p, 1) < 0)
			{
				return -1;
			}
			++p;
		}
		else if (*p == ' ')
		{
			const char* s = p;
			while (*p == ' ')
				++p;
			if (push_token(out_tokens, max_tokens, &total, layout, T_SPACE, s, (int)(p - s)) < 0)
			{
				return -1;
			}
		}
		else if (*p == '$')
		{
			int is_display = 0;
			const char* after_open = p + 1;
			if (*after_open == '$')
			{
				is_display = 1;
				after_open = p + 2;
			}

			const char* close = find_math_end(after_open, is_display);
			if (close)
			{
				int raw_len = (int)(close - after_open);
				TokenType tt = is_display ? T_MATH_DISPLAY : T_MATH_INLINE;
				if (emit_text_token(out_tokens, max_tokens, &total, layout, tt, after_open, raw_len, counting_only) < 0)
				{
					return -1;
				}
				p = close + (is_display ? 2 : 1);
			}
			else
			{
				// unclosed math delimiter: treat '$' as starting a text run
				const char* s = p;
				++p;
				while (*p && *p != ' ' && *p != '\n' && *p != '$')
				{
					if (*p == '\\' && *(p + 1) && tex_util_is_escape_char(*(p + 1)))
						p += 2;
					else
						++p;
				}
				int raw_len = (int)(p - s);
				if (emit_text_token(out_tokens, max_tokens, &total, layout, T_TEXT, s, raw_len, counting_only) < 0)
				{
					return -1;
				}
			}
		}
		else
		{
			// plain text run (stop on space/newline or math start)
			const char* s = p;
			while (*p && *p != ' ' && *p != '\n' && *p != '$')
			{
				if (*p == '\\' && *(p + 1) && tex_util_is_escape_char(*(p + 1)))
					p += 2;
				else
					++p;
			}
			int raw_len = (int)(p - s);
			if (emit_text_token(out_tokens, max_tokens, &total, layout, T_TEXT, s, raw_len, counting_only) < 0)
			{
				return -1;
			}
		}
	}

	// final error check before EOF token
	if (TEX_HAS_ERROR(layout))
	{
		return -1;
	}

	// always terminate with EOF
	if (push_token(out_tokens, max_tokens, &total, layout, T_EOF, p, 0) < 0)
	{
		return -1;
	}

	return total;
}
