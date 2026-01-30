#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>

#include "matrix_core/error.hpp"
#include "matrix_core/matrix.hpp"
#include "matrix_core/row_ops.hpp"

namespace matrix_core
{

// swap rows r1 and r2 of matrix m
inline void apply_swap(MatrixMutView m, std::uint8_t r1, std::uint8_t r2) noexcept
{
	if (r1 == r2)
		return;
	for (std::uint8_t col = 0; col < m.cols; col++)
		std::swap(m.at_mut(r1, col), m.at_mut(r2, col));
}

// scale row r by k: R_r <- k * R_r
inline ErrorCode apply_scale(MatrixMutView m, std::uint8_t row, const Rational& k) noexcept
{
	for (std::uint8_t col = 0; col < m.cols; col++)
	{
		Rational result;
		ErrorCode ec = rational_mul(m.at(row, col), k, &result);
		if (!is_ok(ec))
			return ec;
		m.at_mut(row, col) = result;
	}
	return ErrorCode::Ok;
}

// add a scaled row: R_dst <- R_dst + k * R_src
inline ErrorCode apply_addmul(MatrixMutView m, std::uint8_t dst, std::uint8_t src, const Rational& k) noexcept
{
	for (std::uint8_t col = 0; col < m.cols; col++)
	{
		Rational scaled;
		ErrorCode ec = rational_mul(m.at(src, col), k, &scaled);
		if (!is_ok(ec))
			return ec;
		Rational result;
		ec = rational_add(m.at(dst, col), scaled, &result);
		if (!is_ok(ec))
			return ec;
		m.at_mut(dst, col) = result;
	}
	return ErrorCode::Ok;
}

// observer for tracking row operations during elimination
struct OpObserver
{
	std::size_t target = static_cast<std::size_t>(-1); // 1 based op index to stop after applying
	std::size_t count = 0;
	RowOp last_op{};
	bool has_last = false;

	bool on_op(const RowOp& op) noexcept
	{
		count++;
		last_op = op;
		has_last = true;
		return count != target;
	}
};

} // namespace matrix_core
