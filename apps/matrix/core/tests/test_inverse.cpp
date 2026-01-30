#include "matrix_core/matrix_core.hpp"

#include <cassert>
#include <cstring>

using matrix_core::Arena;
using matrix_core::ErrorCode;
using matrix_core::ExplainOptions;
using matrix_core::Explanation;
using matrix_core::MatrixMutView;
using matrix_core::Rational;
using matrix_core::Slab;
using matrix_core::StepRenderBuffers;

static MatrixMutView mat2(Arena& a, std::int64_t a00, std::int64_t a01, std::int64_t a10, std::int64_t a11)
{
	MatrixMutView m;
	assert(matrix_core::matrix_alloc(a, 2, 2, &m) == ErrorCode::Ok);
	m.at_mut(0, 0) = Rational::from_int(a00);
	m.at_mut(0, 1) = Rational::from_int(a01);
	m.at_mut(1, 0) = Rational::from_int(a10);
	m.at_mut(1, 1) = Rational::from_int(a11);
	return m;
}

int main()
{
	Slab slab;
	assert(slab.init(256 * 1024) == ErrorCode::Ok);

	Arena persist(slab.data(), slab.size() / 2);
	Arena scratch(slab.data() + (slab.size() / 2), slab.size() - (slab.size() / 2));

	// Inverse correctness + steps render.
	{
		MatrixMutView a = mat2(persist, 1, 2, 3, 4);
		MatrixMutView inv;
		assert(matrix_core::matrix_alloc(persist, 2, 2, &inv) == ErrorCode::Ok);

		Explanation expl;
		auto err = matrix_core::op_inverse(a.view(), scratch, inv, &expl, ExplainOptions{ .enable = true, .persist = &persist });
		assert(matrix_core::is_ok(err));
		assert(expl.available());
		assert(expl.step_count() >= 1);

		// A^{-1} = [[-2, 1], [3/2, -1/2]]
		{
			const auto v = inv.at(0, 0);
			assert(v.num() == -2 && v.den() == 1);
		}
		{
			const auto v = inv.at(0, 1);
			assert(v.num() == 1 && v.den() == 1);
		}
		{
			const auto v = inv.at(1, 0);
			assert(v.num() == 3 && v.den() == 2);
		}
		{
			const auto v = inv.at(1, 1);
			assert(v.num() == -1 && v.den() == 2);
		}

		char caption[128];
		char latex[1024];
		StepRenderBuffers bufs{ caption, sizeof(caption), latex, sizeof(latex), &scratch };

		assert(expl.render_step(0, bufs) == ErrorCode::Ok);
		assert(std::strstr(latex, "\\begin{array}{") != nullptr);
		assert(std::strstr(latex, "|") != nullptr);

		for (std::size_t i = 0; i < expl.step_count(); i++)
			assert(expl.render_step(i, bufs) == ErrorCode::Ok);
		assert(expl.render_step(expl.step_count(), bufs) == ErrorCode::StepOutOfRange);
	}

	// Singular matrix.
	{
		MatrixMutView a = mat2(persist, 1, 2, 2, 4);
		MatrixMutView inv;
		assert(matrix_core::matrix_alloc(persist, 2, 2, &inv) == ErrorCode::Ok);
		auto err = matrix_core::op_inverse(a.view(), scratch, inv, nullptr, ExplainOptions{ .enable = false, .persist = nullptr });
		assert(!matrix_core::is_ok(err));
		assert(err.code == ErrorCode::Singular);
	}

	// Not square.
	{
		MatrixMutView a;
		assert(matrix_core::matrix_alloc(persist, 2, 3, &a) == ErrorCode::Ok);
		MatrixMutView out;
		assert(matrix_core::matrix_alloc(persist, 2, 2, &out) == ErrorCode::Ok);
		auto err = matrix_core::op_inverse(a.view(), scratch, out, nullptr, ExplainOptions{ .enable = false, .persist = nullptr });
		assert(!matrix_core::is_ok(err));
		assert(err.code == ErrorCode::NotSquare);
	}

	// Output dimension mismatch.
	{
		MatrixMutView a = mat2(persist, 1, 2, 3, 4);
		MatrixMutView out;
		assert(matrix_core::matrix_alloc(persist, 1, 2, &out) == ErrorCode::Ok);
		auto err = matrix_core::op_inverse(a.view(), scratch, out, nullptr, ExplainOptions{ .enable = false, .persist = nullptr });
		assert(!matrix_core::is_ok(err));
		assert(err.code == ErrorCode::DimensionMismatch);
	}

	// Explanation option validation and persist overflow.
	{
		MatrixMutView a = mat2(persist, 1, 2, 3, 4);
		MatrixMutView inv;
		assert(matrix_core::matrix_alloc(persist, 2, 2, &inv) == ErrorCode::Ok);

		auto err = matrix_core::op_inverse(a.view(), scratch, inv, nullptr, ExplainOptions{ .enable = true, .persist = &persist });
		assert(!matrix_core::is_ok(err));
		assert(err.code == ErrorCode::Internal);

		Explanation expl;
		err = matrix_core::op_inverse(a.view(), scratch, inv, &expl, ExplainOptions{ .enable = true, .persist = nullptr });
		assert(!matrix_core::is_ok(err));
		assert(err.code == ErrorCode::Internal);

		std::uint8_t tiny_buf[8] = {};
		Arena tiny_persist(tiny_buf, sizeof(tiny_buf));
		err = matrix_core::op_inverse(a.view(), scratch, inv, &expl, ExplainOptions{ .enable = true, .persist = &tiny_persist });
		assert(!matrix_core::is_ok(err));
		assert(err.code == ErrorCode::Overflow);
	}

	// Scratch overflow.
	{
		MatrixMutView a = mat2(persist, 1, 2, 3, 4);
		MatrixMutView inv;
		assert(matrix_core::matrix_alloc(persist, 2, 2, &inv) == ErrorCode::Ok);

		std::uint8_t tiny_buf[64] = {};
		Arena tiny_scratch(tiny_buf, sizeof(tiny_buf));
		auto err = matrix_core::op_inverse(a.view(), tiny_scratch, inv, nullptr, ExplainOptions{ .enable = false, .persist = nullptr });
		assert(!matrix_core::is_ok(err));
		assert(err.code == ErrorCode::Overflow);
	}

	return 0;
}
