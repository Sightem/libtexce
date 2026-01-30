#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include "matrix_core/arena.hpp"
#include "matrix_core/config.hpp"
#include "matrix_core/error.hpp"
#include "matrix_core/rational.hpp"

namespace matrix_core
{
struct MatrixView
{
	std::uint8_t rows = 0;
	std::uint8_t cols = 0;
	std::uint8_t stride = 0;
	const Rational* data = nullptr;

	constexpr Dim dim() const noexcept { return { rows, cols }; }

	const Rational& at(std::uint8_t r, std::uint8_t c) const noexcept
	{
		assert(data);
		assert(r < rows);
		assert(c < cols);
		return data[static_cast<std::size_t>(r) * stride + static_cast<std::size_t>(c)];
	}
};

struct MatrixMutView
{
	std::uint8_t rows = 0;
	std::uint8_t cols = 0;
	std::uint8_t stride = 0;
	Rational* data = nullptr;

	MatrixView view() const noexcept { return { rows, cols, stride, data }; }

	constexpr Dim dim() const noexcept { return { rows, cols }; }

	const Rational& at(std::uint8_t r, std::uint8_t c) const noexcept
	{
		assert(data);
		assert(r < rows);
		assert(c < cols);
		return data[static_cast<std::size_t>(r) * stride + static_cast<std::size_t>(c)];
	}

	Rational& at_mut(std::uint8_t r, std::uint8_t c) noexcept
	{
		assert(data);
		assert(r < rows);
		assert(c < cols);
		return data[static_cast<std::size_t>(r) * stride + static_cast<std::size_t>(c)];
	}
};

ErrorCode matrix_alloc(InOut Arena& arena, In std::uint8_t rows, In std::uint8_t cols, Out MatrixMutView* out) noexcept;
ErrorCode matrix_clone(InOut Arena& arena, In MatrixView src, Out MatrixMutView* out) noexcept;
ErrorCode matrix_copy(In MatrixView src, Out MatrixMutView dst) noexcept;
void matrix_fill_zero(Out MatrixMutView m) noexcept;

ErrorCode matrix_add(In MatrixView a, In MatrixView b, Out MatrixMutView out) noexcept;
ErrorCode matrix_sub(In MatrixView a, In MatrixView b, Out MatrixMutView out) noexcept;
ErrorCode matrix_mul(In MatrixView a, In MatrixView b, Out MatrixMutView out) noexcept;
ErrorCode matrix_transpose(In MatrixView a, Out MatrixMutView out) noexcept;

} // namespace matrix_core
