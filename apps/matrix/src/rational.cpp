#include "app.h"

#include <limits.h>

static int64_t i64_abs(int64_t v) { return (v < 0) ? -v : v; }

static int64_t gcd_i64(int64_t a, int64_t b)
{
	a = i64_abs(a);
	b = i64_abs(b);
	if (a == 0)
		return (b == 0) ? 1 : b;
	while (b != 0)
	{
		const int64_t t = a % b;
		a = b;
		b = t;
	}
	return a;
}

static Rational rational_normalize_i64(int64_t num, int64_t den)
{
	if (den == 0)
		return Rational{0, 1};
	if (num == 0)
		return Rational{0, 1};
	if (den < 0)
	{
		num = -num;
		den = -den;
	}

	const int64_t g = gcd_i64(num, den);
	num /= g;
	den /= g;

	if (num < (int64_t)INT32_MIN)
		num = (int64_t)INT32_MIN;
	if (num > (int64_t)INT32_MAX)
		num = (int64_t)INT32_MAX;
	if (den < 1)
		den = 1;
	if (den > (int64_t)INT32_MAX)
		den = (int64_t)INT32_MAX;

	return Rational{(int32_t)num, (int32_t)den};
}

Rational rational_from_int(int32_t value) { return Rational{value, 1}; }

Rational rational_add(Rational a, Rational b)
{
	return rational_normalize_i64((int64_t)a.num * (int64_t)b.den + (int64_t)b.num * (int64_t)a.den,
	                              (int64_t)a.den * (int64_t)b.den);
}

Rational rational_sub(Rational a, Rational b)
{
	return rational_normalize_i64((int64_t)a.num * (int64_t)b.den - (int64_t)b.num * (int64_t)a.den,
	                              (int64_t)a.den * (int64_t)b.den);
}

Rational rational_mul(Rational a, Rational b)
{
	return rational_normalize_i64((int64_t)a.num * (int64_t)b.num, (int64_t)a.den * (int64_t)b.den);
}

Rational rational_div(Rational a, Rational b)
{
	if (b.num == 0)
		return Rational{0, 1};
	return rational_normalize_i64((int64_t)a.num * (int64_t)b.den, (int64_t)a.den * (int64_t)b.num);
}

Rational rational_neg(Rational a) { return Rational{-a.num, a.den}; }

bool rational_is_zero(Rational a) { return a.num == 0; }
