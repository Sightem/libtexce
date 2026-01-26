#include "steps.h"

#include <stdlib.h>
#include <string.h>

typedef struct StrBuf {
	char* data;
	size_t len;
	size_t cap;
} StrBuf;

static void sb_free(StrBuf* sb)
{
	if (!sb)
		return;
	free(sb->data);
	sb->data = NULL;
	sb->len = 0;
	sb->cap = 0;
}

static bool sb_reserve(StrBuf* sb, size_t extra)
{
	if (!sb)
		return false;
	const size_t need = sb->len + extra + 1;
	if (need <= sb->cap)
		return true;
	size_t new_cap = (sb->cap == 0) ? 64 : sb->cap;
	while (new_cap < need)
		new_cap *= 2;
	char* p = (char*)realloc(sb->data, new_cap);
	if (!p)
		return false;
	sb->data = p;
	sb->cap = new_cap;
	return true;
}

static bool sb_append_mem(StrBuf* sb, const char* s, size_t n)
{
	if (!sb || !s)
		return false;
	if (!sb_reserve(sb, n))
		return false;
	memcpy(sb->data + sb->len, s, n);
	sb->len += n;
	sb->data[sb->len] = '\0';
	return true;
}

static bool sb_append_cstr(StrBuf* sb, const char* s)
{
	if (!s)
		s = "";
	return sb_append_mem(sb, s, strlen(s));
}

static bool sb_append_char(StrBuf* sb, char c)
{
	if (!sb_reserve(sb, 1))
		return false;
	sb->data[sb->len++] = c;
	sb->data[sb->len] = '\0';
	return true;
}

static bool sb_append_u32(StrBuf* sb, uint32_t v)
{
	char tmp[10];
	size_t n = 0;
	do
	{
		tmp[n++] = (char)('0' + (v % 10u));
		v /= 10u;
	} while (v != 0u && n < sizeof(tmp));

	for (size_t i = 0; i < n; ++i)
	{
		if (!sb_append_char(sb, tmp[n - 1 - i]))
			return false;
	}
	return true;
}

static bool sb_append_i32(StrBuf* sb, int32_t v)
{
	if (v < 0)
	{
		if (!sb_append_char(sb, '-'))
			return false;
		// cast through uint32_t to avoid UB on INT32_MIN
		const uint32_t mag = (uint32_t)(-(v + 1)) + 1u;
		return sb_append_u32(sb, mag);
	}
	return sb_append_u32(sb, (uint32_t)v);
}

static bool sb_append_rational_tex(StrBuf* sb, Rational r)
{
	if (r.den == 1)
		return sb_append_i32(sb, r.num);

	if (r.num < 0)
	{
		if (!sb_append_char(sb, '-'))
			return false;
		if (!sb_append_cstr(sb, "\\frac{"))
			return false;
		const uint32_t mag = (uint32_t)(-(r.num + 1)) + 1u;
		if (!sb_append_u32(sb, mag))
			return false;
		if (!sb_append_cstr(sb, "}{"))
			return false;
		if (!sb_append_i32(sb, r.den))
			return false;
		return sb_append_char(sb, '}');
	}

	if (!sb_append_cstr(sb, "\\frac{"))
		return false;
	if (!sb_append_i32(sb, r.num))
		return false;
	if (!sb_append_cstr(sb, "}{"))
		return false;
	if (!sb_append_i32(sb, r.den))
		return false;
	return sb_append_char(sb, '}');
}

static char* dup_cstr(const char* s)
{
	if (!s)
		s = "";
	const size_t n = strlen(s);
	char* out = (char*)malloc(n + 1);
	if (!out)
		return NULL;
	memcpy(out, s, n);
	out[n] = '\0';
	return out;
}

void steps_clear(StepsLog* log)
{
	if (!log)
		return;
	for (uint8_t i = 0; i < log->count; ++i)
	{
		free(log->items[i].caption);
		free(log->items[i].latex);
		log->items[i].caption = NULL;
		log->items[i].latex = NULL;
	}
	log->count = 0;
	log->has_steps = false;
	log->truncated = false;
	log->op = OP_NONE;
}

bool steps_begin(StepsLog* log, OperationId op)
{
	if (!log)
		return false;
	steps_clear(log);
	log->has_steps = true;
	log->op = op;
	return true;
}

bool steps_append_tex(StepsLog* log, const char* caption, const char* latex)
{
	if (!log || !latex)
		return false;
	if (log->count >= MATRIX_STEPS_MAX)
	{
		log->truncated = true;
		return false;
	}

	char* cap_copy = dup_cstr(caption);
	char* tex_copy = dup_cstr(latex);
	if (!cap_copy || !tex_copy)
	{
		free(cap_copy);
		free(tex_copy);
		log->truncated = true;
		return false;
	}

	StepItem* it = &log->items[log->count++];
	it->caption = cap_copy;
	it->latex = tex_copy;
	return true;
}

bool steps_append_matrix(StepsLog* log, const char* caption, const MatrixSlot* state)
{
	if (!log || !state || !matrix_is_set(state))
		return false;

	StrBuf sb = {NULL, 0, 0};
	bool ok = true;

	ok = ok && sb_append_cstr(&sb, "\\begin{bmatrix}");
	for (uint8_t r = 0; ok && r < state->rows; ++r)
	{
		for (uint8_t c = 0; ok && c < state->cols; ++c)
		{
			ok = ok && sb_append_rational_tex(&sb, state->cell[r][c]);
			if (c + 1 < state->cols)
				ok = ok && sb_append_cstr(&sb, " & ");
		}
		if (r + 1 < state->rows)
			ok = ok && sb_append_cstr(&sb, " \\\\ ");
	}
	ok = ok && sb_append_cstr(&sb, "\\end{bmatrix}");

	if (!ok || !sb.data)
	{
		sb_free(&sb);
		log->truncated = true;
		return false;
	}

	const bool appended = steps_append_tex(log, caption, sb.data);
	sb_free(&sb);
	return appended;
}
