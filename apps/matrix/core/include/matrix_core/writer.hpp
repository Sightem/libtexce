#pragma once

#include <cstddef>
#include <cstdint>

#include "matrix_core/error.hpp"
#include "matrix_core/rational.hpp"

namespace matrix_core
{

// string builder helper that writes to a fixed capacity buffer
// all operations are noexcept and return ErrorCode on overflow
struct Writer
{
	char* data = nullptr;
	std::size_t cap = 0;
	std::size_t len = 0;

	ErrorCode put(char ch) noexcept
	{
		if (!data || cap == 0)
			return ErrorCode::BufferTooSmall;
		if (len + 1 >= cap)
			return ErrorCode::BufferTooSmall;
		data[len++] = ch;
		data[len] = '\0';
		return ErrorCode::Ok;
	}

	ErrorCode append(const char* s) noexcept
	{
		if (!s)
			return ErrorCode::Internal;
		for (std::size_t i = 0; s[i] != '\0'; i++)
		{
			ErrorCode ec = put(s[i]);
			if (!is_ok(ec))
				return ec;
		}
		return ErrorCode::Ok;
	}

	ErrorCode append_u64(std::uint64_t v) noexcept
	{
		char buf[32];
		std::size_t n = 0;
		do
		{
			buf[n++] = static_cast<char>('0' + (v % 10u));
			v /= 10u;
		} while (v != 0u);

		for (std::size_t i = 0; i < n; i++)
		{
			ErrorCode ec = put(buf[n - 1 - i]);
			if (!is_ok(ec))
				return ec;
		}
		return ErrorCode::Ok;
	}

	ErrorCode append_i64(std::int64_t v) noexcept
	{
		std::uint64_t mag = (v < 0) ? (static_cast<std::uint64_t>(-(v + 1)) + 1u) : static_cast<std::uint64_t>(v);
		if (v < 0)
		{
			ErrorCode ec = put('-');
			if (!is_ok(ec))
				return ec;
		}
		return append_u64(mag);
	}

	// append a 1 based index (v+1) as decimal
	ErrorCode append_index1(std::uint8_t v) noexcept
	{
		return append_u64(static_cast<std::uint64_t>(v) + 1u);
	}

	// append a rational as LaTeX (either integer or \frac{num}{den})
	ErrorCode append_rational_latex(const Rational& r) noexcept
	{
		if (r.den() == 1)
			return append_i64(r.num());

		ErrorCode ec = append("\\frac{");
		if (!is_ok(ec))
			return ec;
		ec = append_i64(r.num());
		if (!is_ok(ec))
			return ec;
		ec = append("}{");
		if (!is_ok(ec))
			return ec;
		ec = append_i64(r.den());
		if (!is_ok(ec))
			return ec;
		return append("}");
	}
};

} // namespace matrix_core
