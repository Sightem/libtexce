#include "matrix_core/matrix_core.hpp"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

using matrix_core::Arena;
using matrix_core::ErrorCode;
using matrix_core::ExplainOptions;
using matrix_core::Explanation;
using matrix_core::MatrixMutView;
using matrix_core::Rational;
using matrix_core::Slab;
using matrix_core::StepRenderBuffers;

int main()
{
	Slab slab;
	assert(slab.init(256 * 1024) == ErrorCode::Ok);

	Arena persist(slab.data(), slab.size() / 2);
	Arena scratch(slab.data() + (slab.size() / 2), slab.size() - (slab.size() / 2));

	// System:
	// -x1 - 4x2 + 2x3 + x4 = -32
	//  2x1 -  x2 + 7x3 + 9x4 = 14
	// -x1 +  x2 + 3x3 + x4 = 11
	//  x1 - 2x2 + x3 - 4x4 = -4
	//
	// A x = b
	MatrixMutView a;
	assert(matrix_core::matrix_alloc(persist, 4, 4, &a) == ErrorCode::Ok);
	const std::int64_t avals[4][4] = {
		{ -1, -4, 2, 1 },
		{ 2, -1, 7, 9 },
		{ -1, 1, 3, 1 },
		{ 1, -2, 1, -4 },
	};
	for (std::uint8_t r = 0; r < 4; r++)
	{
		for (std::uint8_t c = 0; c < 4; c++)
			a.at_mut(r, c) = Rational::from_int(avals[r][c]);
	}

	MatrixMutView b;
	assert(matrix_core::matrix_alloc(persist, 4, 1, &b) == ErrorCode::Ok);
	const std::int64_t bvals[4] = { -32, 14, 11, -4 };
	for (std::uint8_t r = 0; r < 4; r++)
		b.at_mut(r, 0) = Rational::from_int(bvals[r]);

	MatrixMutView x;
	assert(matrix_core::matrix_alloc(persist, 4, 1, &x) == ErrorCode::Ok);

	auto err = matrix_core::op_cramer_solve(a.view(), b.view(), scratch, x);
	assert(matrix_core::is_ok(err));

	const std::int64_t expected_x[4] = { 5, 8, 3, -1 };
	for (std::uint8_t r = 0; r < 4; r++)
	{
		const auto v = x.at(r, 0);
		assert(v.den() == 1);
		assert(v.num() == expected_x[r]);
	}

	// "Steps" for Cramer are determinant explanations:
	// Δ = det(A) and Δ_i = det(A_i) for column replacements.
	{
		Rational delta;
		Explanation expl;
		err = matrix_core::op_det(a.view(), scratch, &delta, &expl, ExplainOptions{ .enable = true, .persist = &persist });
		assert(matrix_core::is_ok(err));
		assert(delta.den() == 1 && delta.num() == -423);
		assert(expl.available());

		char caption[128];
		char latex[512];
		StepRenderBuffers bufs{ caption, sizeof(caption), latex, sizeof(latex), &scratch };

			const std::size_t nsteps = expl.step_count();
			assert(nsteps >= 2);
			assert(expl.render_step(0, bufs) == ErrorCode::Ok);
			assert(std::strstr(latex, "\\begin{vmatrix}") != nullptr);
			if (nsteps > 2)
			{
				assert(expl.render_step(1, bufs) == ErrorCode::Ok);
				assert(std::strlen(caption) > 0);
			}
			assert(expl.render_step(nsteps - 1, bufs) == ErrorCode::Ok);
			assert(std::strcmp(latex, "\\det(A) = -423") == 0);

			for (std::size_t i = 0; i < nsteps; i++)
				assert(expl.render_step(i, bufs) == ErrorCode::Ok);
		}

	{
		const std::int64_t expected_delta_i[4] = { -2115, -3384, -1269, 423 };
		for (std::uint8_t col = 0; col < 4; col++)
		{
			Rational delta_i;
			Explanation expl;
			err = matrix_core::op_det_replace_column(
				a.view(), b.view(), col, scratch, &delta_i, &expl, ExplainOptions{ .enable = true, .persist = &persist });
			assert(matrix_core::is_ok(err));
			assert(delta_i.den() == 1 && delta_i.num() == expected_delta_i[col]);
			assert(expl.available());

			char caption[128];
			char latex[512];
			StepRenderBuffers bufs{ caption, sizeof(caption), latex, sizeof(latex), &scratch };
				const std::size_t nsteps = expl.step_count();
				assert(nsteps >= 2);
				assert(expl.render_step(0, bufs) == ErrorCode::Ok);
				assert(std::strstr(latex, "\\begin{vmatrix}") != nullptr);
				if (nsteps > 2)
				{
					assert(expl.render_step(1, bufs) == ErrorCode::Ok);
					assert(std::strlen(caption) > 0);

					char tiny_caption[1];
					StepRenderBuffers tiny_caption_bufs{ tiny_caption, sizeof(tiny_caption), latex, sizeof(latex), &scratch };
					assert(expl.render_step(1, tiny_caption_bufs) == ErrorCode::BufferTooSmall);

					StepRenderBuffers no_caption{ nullptr, 0, latex, sizeof(latex), &scratch };
					assert(expl.render_step(1, no_caption) == ErrorCode::Ok);
				}
				assert(expl.render_step(nsteps - 1, bufs) == ErrorCode::Ok);

				char expected[64];
				std::snprintf(expected, sizeof(expected), "\\det(A_{%u}) = %lld", static_cast<unsigned>(col + 1), static_cast<long long>(expected_delta_i[col]));
				assert(std::strcmp(latex, expected) == 0);

				for (std::size_t i = 0; i < nsteps; i++)
					assert(expl.render_step(i, bufs) == ErrorCode::Ok);
				assert(expl.render_step(nsteps, bufs) == ErrorCode::StepOutOfRange);

				// Render-step requires scratch; and final-step buffer sizing matters.
				StepRenderBuffers no_scratch{ caption, sizeof(caption), latex, sizeof(latex), nullptr };
				assert(expl.render_step(0, no_scratch) == ErrorCode::Internal);
			StepRenderBuffers tiny{ nullptr, 0, latex, 8, &scratch };
			assert(expl.render_step(nsteps - 1, tiny) == ErrorCode::BufferTooSmall);
		}
	}

	// op_cramer_solve error cases.
		{
			MatrixMutView ns;
			assert(matrix_core::matrix_alloc(persist, 2, 3, &ns) == ErrorCode::Ok);
			MatrixMutView bb;
			assert(matrix_core::matrix_alloc(persist, 2, 1, &bb) == ErrorCode::Ok);
			MatrixMutView xx;
			assert(matrix_core::matrix_alloc(persist, 2, 1, &xx) == ErrorCode::Ok);
			auto err2 = matrix_core::op_cramer_solve(ns.view(), bb.view(), scratch, xx);
			assert(!matrix_core::is_ok(err2));
			assert(err2.code == ErrorCode::NotSquare);
		}
		{
			MatrixMutView bad_b;
			assert(matrix_core::matrix_alloc(persist, 1, 4, &bad_b) == ErrorCode::Ok);
			auto err2 = matrix_core::op_cramer_solve(a.view(), bad_b.view(), scratch, x);
			assert(!matrix_core::is_ok(err2));
			assert(err2.code == ErrorCode::DimensionMismatch);
		}
		{
			MatrixMutView bad_x;
			assert(matrix_core::matrix_alloc(persist, 4, 2, &bad_x) == ErrorCode::Ok);
			auto err2 = matrix_core::op_cramer_solve(a.view(), b.view(), scratch, bad_x);
			assert(!matrix_core::is_ok(err2));
			assert(err2.code == ErrorCode::DimensionMismatch);
		}
	{
		MatrixMutView sing;
		assert(matrix_core::matrix_alloc(persist, 2, 2, &sing) == ErrorCode::Ok);
		sing.at_mut(0, 0) = Rational::from_int(1);
		sing.at_mut(0, 1) = Rational::from_int(2);
		sing.at_mut(1, 0) = Rational::from_int(2);
		sing.at_mut(1, 1) = Rational::from_int(4);

		MatrixMutView bb;
		assert(matrix_core::matrix_alloc(persist, 2, 1, &bb) == ErrorCode::Ok);
		bb.at_mut(0, 0) = Rational::from_int(1);
		bb.at_mut(1, 0) = Rational::from_int(1);

			MatrixMutView xx;
			assert(matrix_core::matrix_alloc(persist, 2, 1, &xx) == ErrorCode::Ok);
			auto err2 = matrix_core::op_cramer_solve(sing.view(), bb.view(), scratch, xx);
			assert(!matrix_core::is_ok(err2));
			assert(err2.code == ErrorCode::Singular);
		}
		{
			std::uint8_t tiny_buf[128] = {};
			Arena tiny_scratch(tiny_buf, sizeof(tiny_buf));
			auto err2 = matrix_core::op_cramer_solve(a.view(), b.view(), tiny_scratch, x);
			assert(!matrix_core::is_ok(err2));
			assert(err2.code == ErrorCode::Overflow);
		}

	// op_det_replace_column argument validation.
	{
		MatrixMutView bad_b;
		assert(matrix_core::matrix_alloc(persist, 4, 2, &bad_b) == ErrorCode::Ok);
		Rational out;
		auto err2 = matrix_core::op_det_replace_column(a.view(), bad_b.view(), 0, scratch, &out, nullptr, ExplainOptions{ .enable = false, .persist = nullptr });
		assert(!matrix_core::is_ok(err2));
		assert(err2.code == ErrorCode::DimensionMismatch);

		MatrixMutView ns;
		assert(matrix_core::matrix_alloc(persist, 2, 3, &ns) == ErrorCode::Ok);
		MatrixMutView bb;
		assert(matrix_core::matrix_alloc(persist, 2, 1, &bb) == ErrorCode::Ok);
		err2 = matrix_core::op_det_replace_column(ns.view(), bb.view(), 0, scratch, &out, nullptr, ExplainOptions{ .enable = false, .persist = nullptr });
		assert(!matrix_core::is_ok(err2));
		assert(err2.code == ErrorCode::NotSquare);

		err2 = matrix_core::op_det_replace_column(a.view(), b.view(), 4, scratch, &out, nullptr, ExplainOptions{ .enable = false, .persist = nullptr });
		assert(!matrix_core::is_ok(err2));
		assert(err2.code == ErrorCode::IndexOutOfRange);
		assert(err2.i == 4);

		err2 = matrix_core::op_det_replace_column(a.view(), b.view(), 0, scratch, nullptr, nullptr, ExplainOptions{ .enable = false, .persist = nullptr });
		assert(!matrix_core::is_ok(err2));
		assert(err2.code == ErrorCode::Internal);

		std::uint8_t tiny_buf[128] = {};
		Arena tiny_scratch(tiny_buf, sizeof(tiny_buf));
		err2 = matrix_core::op_det_replace_column(a.view(), b.view(), 0, tiny_scratch, &out, nullptr, ExplainOptions{ .enable = false, .persist = nullptr });
		assert(!matrix_core::is_ok(err2));
		assert(err2.code == ErrorCode::Overflow);
	}

	return 0;
}
