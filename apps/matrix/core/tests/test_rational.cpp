#include "matrix_core/rational.hpp"
#include "matrix_core/latex.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>

using matrix_core::ErrorCode;
using matrix_core::Rational;

static Rational make(std::int64_t n, std::int64_t d)
{
	Rational r;
	ErrorCode ec = Rational::make(n, d, &r);
	assert(ec == ErrorCode::Ok);
	return r;
}

int main()
{
	{
		Rational r = make(2, 4);
		assert(r.num() == 1);
		assert(r.den() == 2);
	}
	{
		Rational r = make(-2, 4);
		assert(r.num() == -1);
		assert(r.den() == 2);
	}
	{
		Rational r = make(2, -4);
		assert(r.num() == -1);
		assert(r.den() == 2);
	}
	{
		Rational a = make(1, 2);
		Rational b = make(1, 3);
		Rational out;
		assert(matrix_core::rational_add(a, b, &out) == ErrorCode::Ok);
		assert(out.num() == 5);
		assert(out.den() == 6);
	}
	{
		Rational a = make(3, 4);
		Rational b = make(2, 3);
		Rational out;
		assert(matrix_core::rational_mul(a, b, &out) == ErrorCode::Ok);
		assert(out.num() == 1);
		assert(out.den() == 2);
	}
	{
		Rational a = make(3, 4);
		Rational b = make(2, 3);
		Rational out;
		assert(matrix_core::rational_div(a, b, &out) == ErrorCode::Ok);
		assert(out.num() == 9);
		assert(out.den() == 8);
	}

	// Error/edge paths.
	{
		Rational r;
		assert(Rational::make(1, 0, &r) == ErrorCode::DivisionByZero);
	}
	{
		Rational r = make(0, 123);
		assert(r.num() == 0);
		assert(r.den() == 1);
	}
	{
		Rational r;
		assert(Rational::make(1, std::numeric_limits<std::int64_t>::min(), &r) == ErrorCode::Overflow);
		assert(Rational::make(std::numeric_limits<std::int64_t>::min(), -1, &r) == ErrorCode::Overflow);
	}
	{
		Rational a = make(std::numeric_limits<std::int64_t>::min(), 1);
		Rational out;
		assert(matrix_core::rational_neg(a, &out) == ErrorCode::Overflow);
		assert(matrix_core::rational_sub(make(0, 1), a, &out) == ErrorCode::Overflow);
	}
	{
		Rational a = make(std::numeric_limits<std::int64_t>::max(), 1);
		Rational b = make(1, 1);
		Rational out;
		assert(matrix_core::rational_add(a, b, &out) == ErrorCode::Overflow);
	}
	{
		Rational a = make(std::numeric_limits<std::int64_t>::max(), 1);
		Rational b = make(2, 1);
		Rational out;
		assert(matrix_core::rational_mul(a, b, &out) == ErrorCode::Overflow);
	}
	{
		Rational a = make(1, 2);
		Rational b = make(0, 1);
		Rational out;
		assert(matrix_core::rational_div(a, b, &out) == ErrorCode::DivisionByZero);
	}
	{
		Rational a = make(1, 2);
		Rational b = make(1, 2);
		assert(matrix_core::rational_add(a, b, nullptr) == ErrorCode::Internal);
		assert(matrix_core::rational_mul(a, b, nullptr) == ErrorCode::Internal);
		assert(matrix_core::rational_div(a, b, nullptr) == ErrorCode::Internal);
		assert(matrix_core::rational_neg(a, nullptr) == ErrorCode::Internal);
	}

	// latex_view.cpp coverage.
	{
		char buf[32];
		Rational r = make(1, 2);
		assert(matrix_core::latex::write_rational(r, { buf, sizeof(buf) }) == ErrorCode::Ok);
		assert(std::strcmp(buf, "\\frac{1}{2}") == 0);
	}
	{
		char buf[8];
		Rational r = Rational::from_int(-5);
		assert(matrix_core::latex::write_rational(r, { buf, sizeof(buf) }) == ErrorCode::Ok);
		assert(std::strcmp(buf, "-5") == 0);
	}
	{
		Rational r = make(1, 2);
		assert(matrix_core::latex::write_rational(r, { nullptr, 0 }) == ErrorCode::BufferTooSmall);
		char buf[4];
		assert(matrix_core::latex::write_rational(r, { buf, sizeof(buf) }) == ErrorCode::BufferTooSmall);
	}
	{
		Rational data[2] = { Rational::from_int(1), make(-3, 2) };
		matrix_core::MatrixView m{ 1, 2, 2, data };

		char buf[128];
		assert(matrix_core::latex::write_matrix(m, matrix_core::latex::MatrixBrackets::PMatrix, { buf, sizeof(buf) }) == ErrorCode::Ok);
		assert(std::strstr(buf, "\\begin{pmatrix}") != nullptr);
		assert(std::strstr(buf, "\\frac{-3}{2}") != nullptr);

		char tiny[8];
		assert(matrix_core::latex::write_matrix(m, matrix_core::latex::MatrixBrackets::PMatrix, { tiny, sizeof(tiny) }) == ErrorCode::BufferTooSmall);

		matrix_core::MatrixView bad{ 1, 1, 1, nullptr };
		assert(matrix_core::latex::write_matrix(bad, matrix_core::latex::MatrixBrackets::BMatrix, { buf, sizeof(buf) }) == ErrorCode::Internal);
	}

	return 0;
}
