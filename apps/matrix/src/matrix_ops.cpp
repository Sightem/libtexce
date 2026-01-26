#include "app.h"
#include "steps.h"

#include <stddef.h>

bool matrix_is_set(const MatrixSlot* m) { return m && m->rows > 0 && m->cols > 0; }

void matrix_clear(MatrixSlot* m)
{
	if (!m)
		return;
	m->rows = 0;
	m->cols = 0;
	for (uint8_t r = 0; r < MATRIX_MAX_ROWS; ++r)
	{
		for (uint8_t c = 0; c < MATRIX_MAX_COLS; ++c)
			m->cell[r][c] = Rational{0, 1};
	}
}

static void matrix_init_dims(MatrixSlot* out, uint8_t rows, uint8_t cols)
{
	out->rows = rows;
	out->cols = cols;
	for (uint8_t r = 0; r < MATRIX_MAX_ROWS; ++r)
	{
		for (uint8_t c = 0; c < MATRIX_MAX_COLS; ++c)
			out->cell[r][c] = Rational{0, 1};
	}
}

bool matrix_add(const MatrixSlot* a, const MatrixSlot* b, MatrixSlot* out)
{
	if (!matrix_is_set(a) || !matrix_is_set(b) || !out)
		return false;
	if (a->rows != b->rows || a->cols != b->cols)
		return false;

	matrix_init_dims(out, a->rows, a->cols);
	for (uint8_t r = 0; r < a->rows; ++r)
	{
		for (uint8_t c = 0; c < a->cols; ++c)
			out->cell[r][c] = rational_add(a->cell[r][c], b->cell[r][c]);
	}
	return true;
}

bool matrix_sub(const MatrixSlot* a, const MatrixSlot* b, MatrixSlot* out)
{
	if (!matrix_is_set(a) || !matrix_is_set(b) || !out)
		return false;
	if (a->rows != b->rows || a->cols != b->cols)
		return false;

	matrix_init_dims(out, a->rows, a->cols);
	for (uint8_t r = 0; r < a->rows; ++r)
	{
		for (uint8_t c = 0; c < a->cols; ++c)
			out->cell[r][c] = rational_sub(a->cell[r][c], b->cell[r][c]);
	}
	return true;
}

bool matrix_mul(const MatrixSlot* left, const MatrixSlot* right, MatrixSlot* out)
{
	if (!matrix_is_set(left) || !matrix_is_set(right) || !out)
		return false;
	if (left->cols != right->rows)
		return false;

	matrix_init_dims(out, left->rows, right->cols);
	for (uint8_t r = 0; r < left->rows; ++r)
	{
		for (uint8_t c = 0; c < right->cols; ++c)
		{
			Rational sum = Rational{0, 1};
			for (uint8_t k = 0; k < left->cols; ++k)
				sum = rational_add(sum, rational_mul(left->cell[r][k], right->cell[k][c]));
			out->cell[r][c] = sum;
		}
	}
	return true;
}

static void matrix_copy(const MatrixSlot* src, MatrixSlot* dst)
{
	matrix_init_dims(dst, src->rows, src->cols);
	for (uint8_t r = 0; r < src->rows; ++r)
	{
		for (uint8_t c = 0; c < src->cols; ++c)
			dst->cell[r][c] = src->cell[r][c];
	}
}

static void row_swap(MatrixSlot* m, uint8_t r1, uint8_t r2)
{
	if (r1 == r2)
		return;
	for (uint8_t c = 0; c < m->cols; ++c)
	{
		const Rational tmp = m->cell[r1][c];
		m->cell[r1][c] = m->cell[r2][c];
		m->cell[r2][c] = tmp;
	}
}

static void row_scale(MatrixSlot* m, uint8_t r, Rational k)
{
	for (uint8_t c = 0; c < m->cols; ++c)
		m->cell[r][c] = rational_mul(m->cell[r][c], k);
}

static void row_add_multiple(MatrixSlot* m, uint8_t dst_r, uint8_t src_r, Rational k)
{
	for (uint8_t c = 0; c < m->cols; ++c)
		m->cell[dst_r][c] = rational_add(m->cell[dst_r][c], rational_mul(k, m->cell[src_r][c]));
}

static bool cap_append_char(char* buf, size_t cap, size_t* len, char ch)
{
	if (!buf || !len || cap == 0)
		return false;
	if (*len + 1 >= cap)
		return false;
	buf[(*len)++] = ch;
	buf[*len] = '\0';
	return true;
}

static bool cap_append_cstr(char* buf, size_t cap, size_t* len, const char* s)
{
	if (!s)
		s = "";
	while (*s)
	{
		if (!cap_append_char(buf, cap, len, *s++))
			return false;
	}
	return true;
}

static bool cap_append_u8(char* buf, size_t cap, size_t* len, uint8_t v)
{
	char tmp[3];
	tmp[0] = (char)('0' + (v / 10));
	tmp[1] = (char)('0' + (v % 10));
	tmp[2] = '\0';
	if (v >= 10)
		return cap_append_cstr(buf, cap, len, tmp);
	return cap_append_char(buf, cap, len, tmp[1]);
}

static bool cap_append_u32(char* buf, size_t cap, size_t* len, uint32_t v);

static bool cap_append_i32(char* buf, size_t cap, size_t* len, int32_t v)
{
	if (v < 0)
	{
		if (!cap_append_char(buf, cap, len, '-'))
			return false;
		const uint32_t mag = (uint32_t)(-(v + 1)) + 1u;
		return cap_append_u32(buf, cap, len, mag);
	}

	return cap_append_u32(buf, cap, len, (uint32_t)v);
}

static bool cap_append_u32(char* buf, size_t cap, size_t* len, uint32_t v)
{
	// max 10 digits for uint32
	char digits[10];
	size_t n = 0;
	uint32_t x = v;
	do
	{
		digits[n++] = (char)('0' + (x % 10u));
		x /= 10u;
	} while (x != 0u && n < sizeof(digits));
	for (size_t i = 0; i < n; ++i)
	{
		if (!cap_append_char(buf, cap, len, digits[n - 1 - i]))
			return false;
	}
	return true;
}

static bool cap_append_rational_mag(char* buf, size_t cap, size_t* len, Rational r)
{
	const uint32_t mag = (r.num < 0) ? ((uint32_t)(-(r.num + 1)) + 1u) : (uint32_t)r.num;
	if (r.den == 1)
		return cap_append_u32(buf, cap, len, mag);

	if (!cap_append_char(buf, cap, len, '('))
		return false;
	if (!cap_append_u32(buf, cap, len, mag))
		return false;
	if (!cap_append_char(buf, cap, len, '/'))
		return false;
	if (!cap_append_i32(buf, cap, len, r.den))
		return false;
	return cap_append_char(buf, cap, len, ')');
}

static void make_caption_swap(char* out, size_t cap, uint8_t r1, uint8_t r2)
{
	size_t n = 0;
	out[0] = '\0';
	(void)cap_append_cstr(out, cap, &n, "R");
	(void)cap_append_u8(out, cap, &n, (uint8_t)(r1 + 1));
	(void)cap_append_cstr(out, cap, &n, " <-> R");
	(void)cap_append_u8(out, cap, &n, (uint8_t)(r2 + 1));
}

static void make_caption_scale(char* out, size_t cap, uint8_t r, Rational k)
{
	size_t n = 0;
	out[0] = '\0';
	(void)cap_append_cstr(out, cap, &n, "R");
	(void)cap_append_u8(out, cap, &n, (uint8_t)(r + 1));
	(void)cap_append_cstr(out, cap, &n, " <- ");
	if (k.num < 0)
		(void)cap_append_char(out, cap, &n, '-');
	(void)cap_append_rational_mag(out, cap, &n, k);
	(void)cap_append_cstr(out, cap, &n, "R");
	(void)cap_append_u8(out, cap, &n, (uint8_t)(r + 1));
}

static void make_caption_addmul(char* out, size_t cap, uint8_t dst, uint8_t src, Rational k)
{
	size_t n = 0;
	out[0] = '\0';
	(void)cap_append_cstr(out, cap, &n, "R");
	(void)cap_append_u8(out, cap, &n, (uint8_t)(dst + 1));
	(void)cap_append_cstr(out, cap, &n, " <- R");
	(void)cap_append_u8(out, cap, &n, (uint8_t)(dst + 1));
	(void)cap_append_char(out, cap, &n, ' ');
	if (k.num < 0)
		(void)cap_append_cstr(out, cap, &n, "- ");
	else
		(void)cap_append_cstr(out, cap, &n, "+ ");
	(void)cap_append_rational_mag(out, cap, &n, k);
	(void)cap_append_cstr(out, cap, &n, "R");
	(void)cap_append_u8(out, cap, &n, (uint8_t)(src + 1));
}

static bool matrix_rref_impl(const MatrixSlot* in, MatrixSlot* out, bool reduced, StepsLog* steps)
{
	if (!matrix_is_set(in) || !out)
		return false;

	matrix_copy(in, out);
	if (steps && steps->has_steps)
		(void)steps_append_matrix(steps, "Start", out);

	uint8_t pivot_row = 0;
	for (uint8_t pivot_col = 0; pivot_col < out->cols; ++pivot_col)
	{
		// find pivot
		uint8_t best = 0xFF;
		for (uint8_t r = pivot_row; r < out->rows; ++r)
		{
			if (!rational_is_zero(out->cell[r][pivot_col]))
			{
				best = r;
				break;
			}
		}
		if (best == 0xFF)
			continue;

		// swap pivot row into place
		if (best != pivot_row)
		{
			row_swap(out, pivot_row, best);
			if (steps && steps->has_steps)
			{
				char cap[24];
				make_caption_swap(cap, sizeof(cap), pivot_row, best);
				(void)steps_append_matrix(steps, cap, out);
			}
		}

		// scale pivot row to make pivot == 1
		const Rational pivot = out->cell[pivot_row][pivot_col];
		if (!(pivot.den == 1 && pivot.num == 1))
		{
			const Rational scale = rational_div(Rational{1, 1}, pivot);
			row_scale(out, pivot_row, scale);
			if (steps && steps->has_steps)
			{
				char cap[32];
				make_caption_scale(cap, sizeof(cap), pivot_row, scale);
				(void)steps_append_matrix(steps, cap, out);
			}
		}

		// eliminate
		for (uint8_t r = 0; r < out->rows; ++r)
		{
			if (r == pivot_row)
				continue;
			if (!reduced && r < pivot_row)
				continue;

			const Rational factor = out->cell[r][pivot_col];
			if (rational_is_zero(factor))
				continue;
			const Rational k = rational_neg(factor);
			row_add_multiple(out, r, pivot_row, k);
			if (steps && steps->has_steps)
			{
				char cap[48];
				make_caption_addmul(cap, sizeof(cap), r, pivot_row, k);
				(void)steps_append_matrix(steps, cap, out);
			}
		}

		++pivot_row;
		if (pivot_row >= out->rows)
			break;
	}

	return true;
}

bool matrix_ref(const MatrixSlot* in, MatrixSlot* out) { return matrix_rref_impl(in, out, false, NULL); }
bool matrix_rref(const MatrixSlot* in, MatrixSlot* out) { return matrix_rref_impl(in, out, true, NULL); }
bool matrix_ref_steps(const MatrixSlot* in, MatrixSlot* out, StepsLog* steps)
{
	return matrix_rref_impl(in, out, false, steps);
}
bool matrix_rref_steps(const MatrixSlot* in, MatrixSlot* out, StepsLog* steps)
{
	return matrix_rref_impl(in, out, true, steps);
}

bool matrix_det(const MatrixSlot* in, Rational* out_det)
{
	if (!matrix_is_set(in) || !out_det)
		return false;
	if (in->rows != in->cols)
		return false;

	const uint8_t n = in->rows;
	MatrixSlot tmp;
	matrix_copy(in, &tmp);

	Rational sign = Rational{1, 1};

	for (uint8_t i = 0; i < n; ++i)
	{
		uint8_t pivot = 0xFF;
		for (uint8_t r = i; r < n; ++r)
		{
			if (!rational_is_zero(tmp.cell[r][i]))
			{
				pivot = r;
				break;
			}
		}

		if (pivot == 0xFF)
		{
			*out_det = Rational{0, 1};
			return true;
		}

		if (pivot != i)
		{
			row_swap(&tmp, i, pivot);
			sign = rational_neg(sign);
		}

		const Rational piv = tmp.cell[i][i];
		for (uint8_t r = (uint8_t)(i + 1); r < n; ++r)
		{
			const Rational a = tmp.cell[r][i];
			if (rational_is_zero(a))
				continue;

			const Rational factor = rational_div(a, piv);
			for (uint8_t c = i; c < n; ++c)
			{
				tmp.cell[r][c] = rational_sub(tmp.cell[r][c], rational_mul(factor, tmp.cell[i][c]));
			}
		}
	}

	Rational det = sign;
	for (uint8_t i = 0; i < n; ++i)
		det = rational_mul(det, tmp.cell[i][i]);

	*out_det = det;
	return true;
}

bool matrix_det_steps(const MatrixSlot* in, Rational* out_det, StepsLog* steps)
{
	if (!matrix_is_set(in) || !out_det)
		return false;
	if (in->rows != in->cols)
		return false;

	const uint8_t n = in->rows;
	MatrixSlot tmp;
	matrix_copy(in, &tmp);

	if (steps && steps->has_steps)
		(void)steps_append_matrix(steps, "Start", &tmp);

	Rational sign = Rational{1, 1};

	for (uint8_t i = 0; i < n; ++i)
	{
		uint8_t pivot = 0xFF;
		for (uint8_t r = i; r < n; ++r)
		{
			if (!rational_is_zero(tmp.cell[r][i]))
			{
				pivot = r;
				break;
			}
		}

		if (pivot == 0xFF)
		{
			*out_det = Rational{0, 1};
			return true;
		}

		if (pivot != i)
		{
			row_swap(&tmp, i, pivot);
			sign = rational_neg(sign);
			if (steps && steps->has_steps)
			{
				char cap[24];
				make_caption_swap(cap, sizeof(cap), i, pivot);
				(void)steps_append_matrix(steps, cap, &tmp);
			}
		}

		const Rational piv = tmp.cell[i][i];
		for (uint8_t r = (uint8_t)(i + 1); r < n; ++r)
		{
			const Rational a = tmp.cell[r][i];
			if (rational_is_zero(a))
				continue;

			const Rational factor = rational_div(a, piv);
			const Rational k = rational_neg(factor);
			row_add_multiple(&tmp, r, i, k);

			if (steps && steps->has_steps)
			{
				char cap[48];
				make_caption_addmul(cap, sizeof(cap), r, i, k);
				(void)steps_append_matrix(steps, cap, &tmp);
			}
		}
	}

	Rational det = sign;
	for (uint8_t i = 0; i < n; ++i)
		det = rational_mul(det, tmp.cell[i][i]);

	*out_det = det;
	return true;
}

CramerStatus matrix_cramer(const MatrixSlot* a, const MatrixSlot* b, MatrixSlot* out)
{
	if (!matrix_is_set(a) || !matrix_is_set(b) || !out)
		return CRAMER_INVALID;
	if (a->rows != a->cols)
		return CRAMER_INVALID;
	if (b->cols != 1 || b->rows != a->rows)
		return CRAMER_INVALID;

	Rational det_a = Rational{0, 1};
	if (!matrix_det(a, &det_a))
		return CRAMER_INVALID;
	if (rational_is_zero(det_a))
		return CRAMER_SINGULAR;

	matrix_init_dims(out, a->rows, 1);

	const uint8_t n = a->rows;
	for (uint8_t col = 0; col < n; ++col)
	{
		MatrixSlot tmp;
		matrix_copy(a, &tmp);
		for (uint8_t r = 0; r < n; ++r)
			tmp.cell[r][col] = b->cell[r][0];

		Rational det_col = Rational{0, 1};
		if (!matrix_det(&tmp, &det_col))
			return CRAMER_INVALID;

		out->cell[col][0] = rational_div(det_col, det_a);
	}

	return CRAMER_OK;
}
