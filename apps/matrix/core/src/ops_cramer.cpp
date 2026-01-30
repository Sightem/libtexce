#include "matrix_core/ops.hpp"

#include "matrix_core/det_detail.hpp"

namespace matrix_core
{
namespace
{
Error det_of(MatrixView a, Arena& scratch, Rational* out_det) noexcept
{
	if (!out_det)
		return { ErrorCode::Internal };

	ArenaScratchScope scratch_scope(scratch);
	MatrixMutView work;
	ErrorCode ec = matrix_clone(scratch, a, &work);
	if (!is_ok(ec))
		return { ec };

	Rational det;
	ec = detail::det_elim(work, &det, nullptr, static_cast<std::size_t>(-1), nullptr);
	if (!is_ok(ec))
		return { ec };

	*out_det = det;
	return {};
}

Error det_replace_col_of(MatrixView a, MatrixView b, std::uint8_t col, Arena& scratch, Rational* out_det) noexcept
{
	if (!out_det)
		return { ErrorCode::Internal };

	ArenaScratchScope scratch_scope(scratch);
	MatrixMutView work;
	ErrorCode ec = matrix_clone(scratch, a, &work);
	if (!is_ok(ec))
		return { ec };

	for (std::uint8_t row = 0; row < a.rows; row++)
		work.at_mut(row, col) = b.at(row, 0);

	Rational det;
	ec = detail::det_elim(work, &det, nullptr, static_cast<std::size_t>(-1), nullptr);
	if (!is_ok(ec))
		return { ec };

	*out_det = det;
	return {};
}

} // namespace

Error op_cramer_solve(MatrixView a, MatrixView b, Arena& scratch, MatrixMutView x_out) noexcept
{
	Error err;
	if (a.rows != a.cols)
		return err_not_square(a.dim());
	if (b.rows != a.rows || b.cols != 1)
		return err_dim_mismatch(a.dim(), b.dim());
	if (x_out.rows != a.rows || x_out.cols != 1)
		return err_dim_mismatch(a.dim(), x_out.dim());

	Rational delta;
	err = det_of(a, scratch, &delta);
	if (!is_ok(err))
	{
		err.a = a.dim();
		return err;
	}
	if (delta.is_zero())
		return err_singular(a.dim());

	for (std::uint8_t col = 0; col < a.cols; col++)
	{
		Rational delta_i;
		err = det_replace_col_of(a, b, col, scratch, &delta_i);
		if (!is_ok(err))
		{
			err.a = a.dim();
			return err;
		}

		Rational x_i;
		ErrorCode ec = rational_div(delta_i, delta, &x_i);
		if (!is_ok(ec))
		{
			err.code = ec;
			err.a = a.dim();
			return err;
		}
		x_out.at_mut(col, 0) = x_i;
	}

	return err;
}

} // namespace matrix_core
