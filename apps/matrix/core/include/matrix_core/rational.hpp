#pragma once

#include <cstdint>

#include "matrix_core/error.hpp"

namespace matrix_core
{
class Rational
{
public:
	constexpr Rational() noexcept = default;

	static constexpr Rational from_int(std::int64_t v) noexcept { return Rational(v, 1); }

	static ErrorCode make(In std::int64_t num, In std::int64_t den, Out Rational* out) noexcept;

	constexpr std::int64_t num() const noexcept { return num_; }
	constexpr std::int64_t den() const noexcept { return den_; }

	constexpr bool is_zero() const noexcept { return num_ == 0; }

private:
	std::int64_t num_ = 0;
	std::int64_t den_ = 1;

	constexpr Rational(std::int64_t num, std::int64_t den) noexcept : num_(num), den_(den) {}
};

ErrorCode rational_add(In const Rational& a, In const Rational& b, Out Rational* out) noexcept;
ErrorCode rational_sub(In const Rational& a, In const Rational& b, Out Rational* out) noexcept;
ErrorCode rational_mul(In const Rational& a, In const Rational& b, Out Rational* out) noexcept;
ErrorCode rational_div(In const Rational& a, In const Rational& b, Out Rational* out) noexcept;
ErrorCode rational_neg(In const Rational& a, Out Rational* out) noexcept;

} // namespace matrix_core

