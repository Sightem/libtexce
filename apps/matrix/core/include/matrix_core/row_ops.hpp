#pragma once

#include <cstddef>
#include <cstdint>

#include "matrix_core/error.hpp"
#include "matrix_core/rational.hpp"

namespace matrix_core
{
enum class RowOpKind : std::uint8_t
{
	Swap,
	AddMul, // R_i <- R_i + k R_j
	Scale, // R_i <- k R_i
};

struct RowOp
{
	RowOpKind kind = RowOpKind::Swap;
	std::uint8_t target_row = 0;
	std::uint8_t source_row = 0;
	Rational scalar = Rational::from_int(0);
};

// human readable caption for a RowOp (1 based row indices)
ErrorCode row_op_caption(In const RowOp& op, Out char* out, In std::size_t cap) noexcept;

} // namespace matrix_core

