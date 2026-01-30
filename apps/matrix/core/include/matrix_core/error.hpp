#pragma once

#include <cstdint>

// parameter direction annotations
#define In
#define Out
#define InOut

namespace matrix_core
{
struct Dim
{
	std::uint8_t rows = 0;
	std::uint8_t cols = 0;
};

enum class ErrorCode : std::uint8_t
{
	Ok = 0,
	InvalidDimension,
	DimensionMismatch,
	NotSquare,
	Singular,
	DivisionByZero,
	Overflow,
	BufferTooSmall,
	IndexOutOfRange,
	StepOutOfRange,
	Internal,
};

struct Error
{
	ErrorCode code = ErrorCode::Ok;
	Dim a{};
	Dim b{};
	std::uint8_t i = 0;
	std::uint8_t j = 0;
};

constexpr bool is_ok(ErrorCode code) noexcept { return code == ErrorCode::Ok; }
constexpr bool is_ok(const Error& err) noexcept { return is_ok(err.code); }

constexpr Error err_dim_mismatch(Dim a, Dim b) noexcept { return { ErrorCode::DimensionMismatch, a, b }; }
constexpr Error err_not_square(Dim a) noexcept { return { ErrorCode::NotSquare, a }; }
constexpr Error err_singular(Dim a) noexcept { return { ErrorCode::Singular, a }; }
constexpr Error err_overflow(void) noexcept { return { ErrorCode::Overflow }; }
constexpr Error err_invalid_dim(Dim a) noexcept { return { ErrorCode::InvalidDimension, a }; }
} // namespace matrix_core

