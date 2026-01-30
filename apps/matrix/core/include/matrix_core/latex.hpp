#pragma once

#include <cstddef>
#include <cstdint>

#include "matrix_core/error.hpp"
#include "matrix_core/rational.hpp"
#include "matrix_core/matrix.hpp"

namespace matrix_core::latex
{
struct Buffer
{
	char* data = nullptr;
	std::size_t cap = 0;
};

ErrorCode write_rational(In const Rational& r, Out Buffer out) noexcept;

enum class MatrixBrackets : std::uint8_t
{
	BMatrix,
	PMatrix,
	VMatrix,
};

ErrorCode write_matrix(In MatrixView m, In MatrixBrackets brackets, Out Buffer out) noexcept;

// writes an augmented matrix [L | R] using the latex array environment
//
// example:
//   \\left[\\begin{array}{rr|rr} ... \\end{array}\\right]
ErrorCode write_augmented_matrix(In MatrixView left, In MatrixView right, Out Buffer out) noexcept;
} // namespace matrix_core::latex
