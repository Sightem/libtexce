#include "matrix_core/matrix_core.hpp"

#include <cassert>
#include <cstring>

using matrix_core::EchelonKind;
using matrix_core::ErrorCode;
using matrix_core::ExplainOptions;
using matrix_core::Arena;
using matrix_core::Explanation;
using matrix_core::MatrixMutView;
using matrix_core::Rational;
using matrix_core::Slab;
using matrix_core::StepRenderBuffers;

int main()
{
	Slab slab;
	assert(slab.init(128 * 1024) == ErrorCode::Ok);

	Arena persist(slab.data(), slab.size() / 2);
	Arena scratch(slab.data() + (slab.size() / 2), slab.size() - (slab.size() / 2));

	MatrixMutView a;
	assert(matrix_core::matrix_alloc(persist, 2, 2, &a) == ErrorCode::Ok);
	a.at_mut(0, 0) = Rational::from_int(1);
	a.at_mut(0, 1) = Rational::from_int(2);
	a.at_mut(1, 0) = Rational::from_int(3);
	a.at_mut(1, 1) = Rational::from_int(4);

	MatrixMutView out;
	assert(matrix_core::matrix_alloc(persist, 2, 2, &out) == ErrorCode::Ok);

	Explanation expl;
	auto err = matrix_core::op_echelon(a.view(), EchelonKind::Rref, out, &expl, ExplainOptions{ .enable = true, .persist = &persist });
	assert(matrix_core::is_ok(err));

	// RREF of [1 2;3 4] is [1 0;0 1]
	assert(out.at(0, 0).num() == 1 && out.at(0, 0).den() == 1);
	assert(out.at(0, 1).num() == 0 && out.at(0, 1).den() == 1);
	assert(out.at(1, 0).num() == 0 && out.at(1, 0).den() == 1);
	assert(out.at(1, 1).num() == 1 && out.at(1, 1).den() == 1);

	assert(expl.available());
	assert(expl.step_count() >= 1);

	char caption[128];
	char latex[512];
	StepRenderBuffers bufs{ caption, sizeof(caption), latex, sizeof(latex), &scratch };
	assert(expl.render_step(0, bufs) == ErrorCode::Ok);
	assert(expl.render_step(1, bufs) == ErrorCode::Ok);
	assert(std::strlen(caption) > 0);
	assert(std::strstr(latex, "\\begin{bmatrix}") != nullptr);
	assert(expl.render_step(expl.step_count(), bufs) == ErrorCode::StepOutOfRange);

	for (std::size_t i = 0; i < expl.step_count(); i++)
		assert(expl.render_step(i, bufs) == ErrorCode::Ok);

	// Render-step requires scratch.
	StepRenderBuffers no_scratch{ caption, sizeof(caption), latex, sizeof(latex), nullptr };
	assert(expl.render_step(1, no_scratch) == ErrorCode::Internal);

	// Caption buffer too small.
	char tiny_caption[1];
	StepRenderBuffers tiny_caption_bufs{ tiny_caption, sizeof(tiny_caption), latex, sizeof(latex), &scratch };
	assert(expl.render_step(1, tiny_caption_bufs) == ErrorCode::BufferTooSmall);

	// Exercise REF path + row swaps / no-pivot columns.
	{
		MatrixMutView m;
		assert(matrix_core::matrix_alloc(persist, 2, 2, &m) == ErrorCode::Ok);
		m.at_mut(0, 0) = Rational::from_int(0);
		m.at_mut(0, 1) = Rational::from_int(1);
		m.at_mut(1, 0) = Rational::from_int(1);
		m.at_mut(1, 1) = Rational::from_int(0);

			MatrixMutView out2;
			assert(matrix_core::matrix_alloc(persist, 2, 2, &out2) == ErrorCode::Ok);
			Explanation expl2;
			auto err2 = matrix_core::op_echelon(m.view(), EchelonKind::Ref, out2, &expl2, ExplainOptions{ .enable = true, .persist = &persist });
			assert(matrix_core::is_ok(err2));
			assert(expl2.available());

		// Should swap to get a pivot in column 1.
			assert(expl2.step_count() >= 2);
			assert(expl2.render_step(1, bufs) == ErrorCode::Ok);
			assert(std::strstr(caption, "<->") != nullptr);

			for (std::size_t i = 0; i < expl2.step_count(); i++)
				assert(expl2.render_step(i, bufs) == ErrorCode::Ok);
		}

	{
		MatrixMutView m;
		assert(matrix_core::matrix_alloc(persist, 2, 2, &m) == ErrorCode::Ok);
		m.at_mut(0, 0) = Rational::from_int(0);
		m.at_mut(0, 1) = Rational::from_int(1);
		m.at_mut(1, 0) = Rational::from_int(0);
		m.at_mut(1, 1) = Rational::from_int(2);

			MatrixMutView out2;
			assert(matrix_core::matrix_alloc(persist, 2, 2, &out2) == ErrorCode::Ok);
			auto err2 = matrix_core::op_echelon(m.view(), EchelonKind::Ref, out2, nullptr, ExplainOptions{ .enable = false, .persist = nullptr });
			assert(matrix_core::is_ok(err2));
			// First column has no pivot; second does.
			assert(out2.at(0, 1).num() != 0);
	}

	// Output dimension mismatch.
		{
			MatrixMutView bad_out;
			assert(matrix_core::matrix_alloc(persist, 1, 2, &bad_out) == ErrorCode::Ok);
			auto err2 = matrix_core::op_echelon(a.view(), EchelonKind::Ref, bad_out, nullptr, ExplainOptions{ .enable = false, .persist = nullptr });
			assert(!matrix_core::is_ok(err2));
			assert(err2.code == ErrorCode::DimensionMismatch);
		}

	return 0;
}
