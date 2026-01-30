#include "matrix_core/matrix.hpp"

namespace matrix_core
{
namespace
{
constexpr bool valid_dim(std::uint8_t rows, std::uint8_t cols) noexcept
{
	return rows >= 1 && rows <= kMaxRows && cols >= 1 && cols <= kMaxCols;
}
} // namespace

ErrorCode matrix_alloc(Arena& arena, std::uint8_t rows, std::uint8_t cols, MatrixMutView* out) noexcept
{
	if (!out)
		return ErrorCode::Internal;
	if (!valid_dim(rows, cols))
		return ErrorCode::InvalidDimension;

	const std::size_t count = static_cast<std::size_t>(rows) * static_cast<std::size_t>(cols);
	void* mem = arena.allocate(sizeof(Rational) * count, alignof(Rational));
	if (!mem)
		return ErrorCode::Overflow;

	auto* data = static_cast<Rational*>(mem);
	for (std::size_t i = 0; i < count; i++)
		data[i] = Rational::from_int(0);

	out->rows = rows;
	out->cols = cols;
	out->stride = cols;
	out->data = data;
	return ErrorCode::Ok;
}

ErrorCode matrix_clone(Arena& arena, MatrixView src, MatrixMutView* out) noexcept
{
	if (!out)
		return ErrorCode::Internal;
	MatrixMutView dst;
	ErrorCode ec = matrix_alloc(arena, src.rows, src.cols, &dst);
	if (!is_ok(ec))
		return ec;
	ec = matrix_copy(src, dst);
	if (!is_ok(ec))
		return ec;
	*out = dst;
	return ErrorCode::Ok;
}

ErrorCode matrix_copy(MatrixView src, MatrixMutView dst) noexcept
{
	if (!src.data || !dst.data)
		return ErrorCode::Internal;
	if (src.rows != dst.rows || src.cols != dst.cols)
		return ErrorCode::DimensionMismatch;

	for (std::uint8_t row = 0; row < src.rows; row++)
	{
		for (std::uint8_t col = 0; col < src.cols; col++)
			dst.at_mut(row, col) = src.at(row, col);
	}
	return ErrorCode::Ok;
}

void matrix_fill_zero(MatrixMutView m) noexcept
{
	if (!m.data)
		return;
	for (std::uint8_t row = 0; row < m.rows; row++)
	{
		for (std::uint8_t col = 0; col < m.cols; col++)
			m.at_mut(row, col) = Rational::from_int(0);
	}
}

ErrorCode matrix_add(MatrixView a, MatrixView b, MatrixMutView out) noexcept
{
	if (a.rows != b.rows || a.cols != b.cols)
		return ErrorCode::DimensionMismatch;
	if (out.rows != a.rows || out.cols != a.cols)
		return ErrorCode::DimensionMismatch;

	for (std::uint8_t row = 0; row < a.rows; row++)
	{
		for (std::uint8_t col = 0; col < a.cols; col++)
		{
			Rational result;
			ErrorCode ec = rational_add(a.at(row, col), b.at(row, col), &result);
			if (!is_ok(ec))
				return ec;
			out.at_mut(row, col) = result;
		}
	}
	return ErrorCode::Ok;
}

ErrorCode matrix_sub(MatrixView a, MatrixView b, MatrixMutView out) noexcept
{
	if (a.rows != b.rows || a.cols != b.cols)
		return ErrorCode::DimensionMismatch;
	if (out.rows != a.rows || out.cols != a.cols)
		return ErrorCode::DimensionMismatch;

	for (std::uint8_t row = 0; row < a.rows; row++)
	{
		for (std::uint8_t col = 0; col < a.cols; col++)
		{
			Rational result;
			ErrorCode ec = rational_sub(a.at(row, col), b.at(row, col), &result);
			if (!is_ok(ec))
				return ec;
			out.at_mut(row, col) = result;
		}
	}
	return ErrorCode::Ok;
}

ErrorCode matrix_mul(MatrixView a, MatrixView b, MatrixMutView out) noexcept
{
	if (a.cols != b.rows)
		return ErrorCode::DimensionMismatch;
	if (out.rows != a.rows || out.cols != b.cols)
		return ErrorCode::DimensionMismatch;

	for (std::uint8_t i = 0; i < out.rows; i++)
	{
		for (std::uint8_t j = 0; j < out.cols; j++)
		{
			Rational sum = Rational::from_int(0);
			for (std::uint8_t k = 0; k < a.cols; k++)
			{
				Rational prod;
				ErrorCode ec = rational_mul(a.at(i, k), b.at(k, j), &prod);
				if (!is_ok(ec))
					return ec;
				Rational next;
				ec = rational_add(sum, prod, &next);
				if (!is_ok(ec))
					return ec;
				sum = next;
			}
			out.at_mut(i, j) = sum;
		}
	}
	return ErrorCode::Ok;
}

ErrorCode matrix_transpose(MatrixView a, MatrixMutView out) noexcept
{
	if (out.rows != a.cols || out.cols != a.rows)
		return ErrorCode::DimensionMismatch;

	for (std::uint8_t row = 0; row < a.rows; row++)
	{
		for (std::uint8_t col = 0; col < a.cols; col++)
			out.at_mut(col, row) = a.at(row, col);
	}
	return ErrorCode::Ok;
}

} // namespace matrix_core
