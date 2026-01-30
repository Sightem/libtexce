#include "matrix_core/det_detail.hpp"

#include "matrix_core/row_reduction.hpp"

namespace matrix_core::detail
{

ErrorCode det_elim(MatrixMutView m, Rational* det_out, std::size_t* op_count, std::size_t stop_after, RowOp* last_op) noexcept
{
	if (!det_out || !m.data)
		return ErrorCode::Internal;
	if (m.rows != m.cols)
		return ErrorCode::NotSquare;

	const std::uint8_t n = m.rows;
	std::int32_t sign = 1;
	std::size_t ops = 0;
	RowOp last{};
	bool have_last = false;

	for (std::uint8_t col = 0; col < n; col++)
	{
		// find pivot row
		std::uint8_t pivot = col;
		bool found = false;
		for (std::uint8_t row = col; row < n; row++)
		{
			if (!m.at(row, col).is_zero())
			{
				pivot = row;
				found = true;
				break;
			}
		}
		if (!found)
		{
			*det_out = Rational::from_int(0);
			if (op_count)
				*op_count = ops;
			return ErrorCode::Ok;
		}

		if (pivot != col)
		{
			apply_swap(m, col, pivot);
			sign = -sign;

			ops++;
			last.kind = RowOpKind::Swap;
			last.target_row = col;
			last.source_row = pivot;
			have_last = true;
			if (ops == stop_after)
				break;
		}

		Rational pivot_val = m.at(col, col);
		for (std::uint8_t row = col + 1; row < n; row++)
		{
			Rational below = m.at(row, col);
			if (below.is_zero())
				continue;

			// R_row <- R_row - (below/pivot) R_col
			Rational factor;
			ErrorCode ec = rational_div(below, pivot_val, &factor);
			if (!is_ok(ec))
				return ec;
			ec = rational_neg(factor, &factor);
			if (!is_ok(ec))
				return ec;
			ec = apply_addmul(m, row, col, factor);
			if (!is_ok(ec))
				return ec;

			ops++;
			last.kind = RowOpKind::AddMul;
			last.target_row = row;
			last.source_row = col;
			last.scalar = factor;
			have_last = true;
			if (ops == stop_after)
				break;
		}
		if (ops == stop_after)
			break;
	}

	if (op_count)
		*op_count = ops;
	if (last_op && have_last)
		*last_op = last;

	// if we stopped early, det isnt requested/meaningful
	if (ops == stop_after)
	{
		*det_out = Rational::from_int(0);
		return ErrorCode::Ok;
	}

	Rational det = Rational::from_int(sign);
	for (std::uint8_t i = 0; i < n; i++)
	{
		Rational next;
		ErrorCode ec = rational_mul(det, m.at(i, i), &next);
		if (!is_ok(ec))
			return ec;
		det = next;
	}
	*det_out = det;
	return ErrorCode::Ok;
}

} // namespace matrix_core::detail
