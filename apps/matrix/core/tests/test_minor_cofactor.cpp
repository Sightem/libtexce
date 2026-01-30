#include "matrix_core/matrix_core.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>

using matrix_core::Arena;
using matrix_core::ErrorCode;
using matrix_core::ExplainOptions;
using matrix_core::Explanation;
using matrix_core::MatrixMutView;
using matrix_core::MinorCofactorMode;
using matrix_core::Rational;
using matrix_core::Slab;
using matrix_core::StepRenderBuffers;

static MatrixMutView mat4(Arena& a)
{
	MatrixMutView m;
	assert(matrix_core::matrix_alloc(a, 4, 4, &m) == ErrorCode::Ok);
	return m;
}

int main()
{
	Slab slab;
	assert(slab.init(256 * 1024) == ErrorCode::Ok);

	Arena persist(slab.data(), slab.size() / 2);
	Arena scratch(slab.data() + (slab.size() / 2), slab.size() - (slab.size() / 2));

	// A =
	// [ 4 -1  1  6 ]
	// [ 0  0 -3  3 ]
	// [ 4  1  0 14 ]
	// [ 4  1  3  2 ]
	MatrixMutView a = mat4(persist);
	const std::int64_t avals[4][4] = {
		{ 4, -1, 1, 6 },
		{ 0, 0, -3, 3 },
		{ 4, 1, 0, 14 },
		{ 4, 1, 3, 2 },
	};
	for (std::uint8_t r = 0; r < 4; r++)
	{
		for (std::uint8_t c = 0; c < 4; c++)
			a.at_mut(r, c) = Rational::from_int(avals[r][c]);
	}

	const std::int64_t expected_minor[4][4] = {
		{ -27, -108, 0, 0 },
		{ 72, -48, -96, 24 },
		{ 36, 24, -24, 24 },
		{ 63, -84, -24, 24 },
	};
	const std::int64_t expected_cof[4][4] = {
		{ -27, 108, 0, 0 },
		{ -72, -48, 96, 24 },
		{ 36, -24, -24, -24 },
		{ -63, -84, 24, 24 },
	};

	for (std::uint8_t r = 0; r < 4; r++)
	{
		for (std::uint8_t c = 0; c < 4; c++)
		{
			Rational m;
			Rational cof;
			auto err = matrix_core::op_minor_cofactor(
				a.view(), r, c, MinorCofactorMode::Both, scratch, &m, &cof, nullptr, ExplainOptions{ .enable = false, .persist = nullptr });
			assert(matrix_core::is_ok(err));

			assert(m.den() == 1);
			assert(cof.den() == 1);
			assert(m.num() == expected_minor[r][c]);
			assert(cof.num() == expected_cof[r][c]);
		}
	}

	// 1x1: minor=1, cofactor=1.
	{
		MatrixMutView one;
		assert(matrix_core::matrix_alloc(persist, 1, 1, &one) == ErrorCode::Ok);
		one.at_mut(0, 0) = Rational::from_int(7);

		Rational m;
		Rational cof;
		Explanation expl;
		auto err = matrix_core::op_minor_cofactor(
			one.view(), 0, 0, MinorCofactorMode::Both, scratch, &m, &cof, &expl, ExplainOptions{ .enable = true, .persist = &persist });
		assert(matrix_core::is_ok(err));
		assert(m.num() == 1 && m.den() == 1);
		assert(cof.num() == 1 && cof.den() == 1);
		assert(expl.available());
		assert(expl.step_count() == 3);

		char caption[64];
		char latex[128];
		StepRenderBuffers bufs{ caption, sizeof(caption), latex, sizeof(latex), &scratch };
		assert(expl.render_step(0, bufs) == ErrorCode::Ok);
		assert(expl.render_step(1, bufs) == ErrorCode::Ok);
		assert(std::strcmp(latex, "M_{1,1} = \\det(A_{(1,1)}) = 1") == 0);
		assert(expl.render_step(2, bufs) == ErrorCode::Ok);
		assert(std::strcmp(latex, "C_{1,1} = (-1)^{2} M_{1,1} = 1") == 0);
	}

	// 1x1 mode coverage.
	{
		MatrixMutView one;
		assert(matrix_core::matrix_alloc(persist, 1, 1, &one) == ErrorCode::Ok);
		one.at_mut(0, 0) = Rational::from_int(7);

		Rational m;
		Explanation expl;
		auto err = matrix_core::op_minor_cofactor(
			one.view(), 0, 0, MinorCofactorMode::Minor, scratch, &m, nullptr, &expl, ExplainOptions{ .enable = true, .persist = &persist });
		assert(matrix_core::is_ok(err));
		assert(m.num() == 1 && m.den() == 1);
		assert(expl.step_count() == 2);

		char latex[128];
		StepRenderBuffers bufs{ nullptr, 0, latex, sizeof(latex), &scratch };
		assert(expl.render_step(1, bufs) == ErrorCode::Ok);
		assert(std::strcmp(latex, "M_{1,1} = \\det(A_{(1,1)}) = 1") == 0);

		Rational cof;
		err = matrix_core::op_minor_cofactor(
			one.view(), 0, 0, MinorCofactorMode::Cofactor, scratch, nullptr, &cof, &expl, ExplainOptions{ .enable = true, .persist = &persist });
		assert(matrix_core::is_ok(err));
		assert(cof.num() == 1 && cof.den() == 1);
		assert(expl.step_count() == 2);
		assert(expl.render_step(1, bufs) == ErrorCode::Ok);
		assert(std::strcmp(latex, "C_{1,1} = (-1)^{2} M_{1,1} = 1") == 0);
	}

	// Mode-specific output validation.
	{
		Rational m;
		auto err = matrix_core::op_minor_cofactor(
			a.view(), 0, 0, MinorCofactorMode::Minor, scratch, &m, nullptr, nullptr, ExplainOptions{ .enable = false, .persist = nullptr });
		assert(matrix_core::is_ok(err));

		Rational cof;
		err = matrix_core::op_minor_cofactor(
			a.view(), 0, 0, MinorCofactorMode::Cofactor, scratch, nullptr, &cof, nullptr, ExplainOptions{ .enable = false, .persist = nullptr });
		assert(matrix_core::is_ok(err));

		err = matrix_core::op_minor_cofactor(
			a.view(), 0, 0, MinorCofactorMode::Minor, scratch, nullptr, nullptr, nullptr, ExplainOptions{ .enable = false, .persist = nullptr });
		assert(!matrix_core::is_ok(err));
		assert(err.code == ErrorCode::Internal);
		err = matrix_core::op_minor_cofactor(
			a.view(), 0, 0, MinorCofactorMode::Cofactor, scratch, nullptr, nullptr, nullptr, ExplainOptions{ .enable = false, .persist = nullptr });
		assert(!matrix_core::is_ok(err));
		assert(err.code == ErrorCode::Internal);
	}

	// Overflow during cofactor sign flip (minor == INT64_MIN).
	{
		MatrixMutView m;
		assert(matrix_core::matrix_alloc(persist, 2, 2, &m) == ErrorCode::Ok);
		matrix_core::matrix_fill_zero(m);
		m.at_mut(1, 0) = Rational::from_int(std::numeric_limits<std::int64_t>::min());

		Rational cof;
		auto err = matrix_core::op_minor_cofactor(
			m.view(), 0, 1, MinorCofactorMode::Cofactor, scratch, nullptr, &cof, nullptr, ExplainOptions{ .enable = false, .persist = nullptr });
		assert(!matrix_core::is_ok(err));
		assert(err.code == ErrorCode::Overflow);
	}

	// Argument validation.
	{
		Rational m;
		Rational cof;
		auto err = matrix_core::op_minor_cofactor(
			a.view(), 4, 0, MinorCofactorMode::Both, scratch, &m, &cof, nullptr, ExplainOptions{ .enable = false, .persist = nullptr });
		assert(!matrix_core::is_ok(err));
		assert(err.code == ErrorCode::IndexOutOfRange);
		assert(err.i == 4);

		MatrixMutView ns;
		assert(matrix_core::matrix_alloc(persist, 2, 3, &ns) == ErrorCode::Ok);
		err = matrix_core::op_minor_cofactor(
			ns.view(), 0, 0, MinorCofactorMode::Both, scratch, &m, &cof, nullptr, ExplainOptions{ .enable = false, .persist = nullptr });
		assert(!matrix_core::is_ok(err));
		assert(err.code == ErrorCode::NotSquare);
	}

	// Spot-check step breakdown for (i,j)=(2,1) in 1-based math, i.e. (1,0) in 0-based.
	{
		Rational m;
		Rational cof;
		Explanation expl;
		auto err = matrix_core::op_minor_cofactor(
			a.view(), 1, 0, MinorCofactorMode::Both, scratch, &m, &cof, &expl, ExplainOptions{ .enable = true, .persist = &persist });
		assert(matrix_core::is_ok(err));
		assert(m.num() == 72 && m.den() == 1);
		assert(cof.num() == -72 && cof.den() == 1);
		assert(expl.available());

		const std::size_t nsteps = expl.step_count();
		assert(nsteps >= 3);

		char caption[128];
		char latex[512];
		StepRenderBuffers bufs{ caption, sizeof(caption), latex, sizeof(latex), &scratch };

		assert(expl.render_step(0, bufs) == ErrorCode::Ok);
		assert(std::strstr(latex, "\\begin{bmatrix}") != nullptr);

		assert(expl.render_step(1, bufs) == ErrorCode::Ok);
		assert(std::strstr(caption, "Delete row") != nullptr);

		char tiny_caption[8];
		StepRenderBuffers tiny_cap{ tiny_caption, sizeof(tiny_caption), latex, sizeof(latex), &scratch };
		assert(expl.render_step(1, tiny_cap) == ErrorCode::BufferTooSmall);

		// If we have row-ops, render at least one elimination step.
		if (nsteps > 4)
		{
			assert(expl.render_step(2, bufs) == ErrorCode::Ok);
			assert(std::strlen(caption) > 0);
		}

		assert(expl.render_step(nsteps - 2, bufs) == ErrorCode::Ok);
		assert(std::strcmp(latex, "M_{2,1} = \\det(A_{(2,1)}) = 72") == 0);

		assert(expl.render_step(nsteps - 1, bufs) == ErrorCode::Ok);
		assert(std::strcmp(latex, "C_{2,1} = (-1)^{3} M_{2,1} = -72") == 0);

		for (std::size_t i = 0; i < nsteps; i++)
			assert(expl.render_step(i, bufs) == ErrorCode::Ok);

		assert(expl.render_step(nsteps, bufs) == ErrorCode::StepOutOfRange);

		StepRenderBuffers no_scratch{ caption, sizeof(caption), latex, sizeof(latex), nullptr };
		assert(expl.render_step(0, no_scratch) == ErrorCode::Internal);

		char tiny_latex[8];
		StepRenderBuffers tiny{ caption, sizeof(caption), tiny_latex, sizeof(tiny_latex), &scratch };
		assert(expl.render_step(nsteps - 2, tiny) == ErrorCode::BufferTooSmall);
	}

	return 0;
}
