#include "matrix_core/matrix_core.hpp"

#include "matrix_core/det_detail.hpp"
#include "matrix_core/row_ops.hpp"

#include <cassert>
#include <cstring>

using matrix_core::ErrorCode;
using matrix_core::ExplainOptions;
using matrix_core::Arena;
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
	{
		Slab slab;
		assert(slab.init(128 * 1024) == ErrorCode::Ok);
		Arena persist(slab.data(), slab.size() / 2);
		Arena scratch(slab.data() + (slab.size() / 2), slab.size() - (slab.size() / 2));

		MatrixMutView a = mat2(persist, 1, 2, 3, 4);
		Rational det;
		Explanation expl;
		auto err = matrix_core::op_det(a.view(), scratch, &det, &expl, ExplainOptions{ .enable = true, .persist = &persist });
		assert(matrix_core::is_ok(err));
		// det = 1*4 - 2*3 = -2
		assert(det.num() == -2);
		assert(det.den() == 1);
		assert(expl.available());

		char caption[128];
		char latex[512];
		StepRenderBuffers bufs{ caption, sizeof(caption), latex, sizeof(latex), &scratch };
		assert(expl.render_step(0, bufs) == ErrorCode::Ok);

		// Render row-op + final value steps for coverage.
		const std::size_t nsteps = expl.step_count();
		assert(nsteps >= 2);
		assert(expl.render_step(1, bufs) == ErrorCode::Ok);
		assert(std::strlen(caption) > 0);
		assert(expl.render_step(nsteps - 1, bufs) == ErrorCode::Ok);
		assert(std::strcmp(latex, "\\det(A) = -2") == 0);

		for (std::size_t i = 0; i < nsteps; i++)
			assert(expl.render_step(i, bufs) == ErrorCode::Ok);

		// Buffer / range errors.
		StepRenderBuffers tiny{ nullptr, 0, latex, 8, &scratch };
		assert(expl.render_step(nsteps - 1, tiny) == ErrorCode::BufferTooSmall);
		assert(expl.render_step(nsteps, bufs) == ErrorCode::StepOutOfRange);
	}

	{
		Slab slab;
		assert(slab.init(64 * 1024) == ErrorCode::Ok);
		Arena persist(slab.data(), slab.size() / 2);
		Arena scratch(slab.data() + (slab.size() / 2), slab.size() - (slab.size() / 2));

		MatrixMutView a = mat2(persist, 1, 2, 2, 4);
		Rational det;
		auto err = matrix_core::op_det(a.view(), scratch, &det, nullptr, ExplainOptions{ .enable = false, .persist = nullptr });
		assert(matrix_core::is_ok(err));
		assert(det.num() == 0);
		assert(det.den() == 1);
	}

	// op_det: not-square error.
	{
		Slab slab;
		assert(slab.init(64 * 1024) == ErrorCode::Ok);
		Arena persist(slab.data(), slab.size() / 2);
		Arena scratch(slab.data() + (slab.size() / 2), slab.size() - (slab.size() / 2));

		MatrixMutView a;
		assert(matrix_core::matrix_alloc(persist, 2, 3, &a) == ErrorCode::Ok);
		Rational det;
		auto err = matrix_core::op_det(a.view(), scratch, &det, nullptr, ExplainOptions{ .enable = false, .persist = nullptr });
		assert(!matrix_core::is_ok(err));
		assert(err.code == ErrorCode::NotSquare);
	}

	// det_detail::det_elim + row_op_caption edge cases.
	{
		Slab slab;
		assert(slab.init(64 * 1024) == ErrorCode::Ok);
		Arena arena(slab.data(), slab.size());

		// Pivot not found => det=0.
		MatrixMutView z;
		assert(matrix_core::matrix_alloc(arena, 2, 2, &z) == ErrorCode::Ok);
		z.at_mut(0, 0) = Rational::from_int(0);
		z.at_mut(0, 1) = Rational::from_int(1);
		z.at_mut(1, 0) = Rational::from_int(0);
		z.at_mut(1, 1) = Rational::from_int(2);

		Rational det{};
		std::size_t ops = 123;
		assert(matrix_core::detail::det_elim(z, &det, &ops, static_cast<std::size_t>(-1), nullptr) == ErrorCode::Ok);
		assert(det.num() == 0 && det.den() == 1);
		assert(ops == 0);

		// Swap required (det=-1) + stop_after early exit.
		MatrixMutView p_stop;
		assert(matrix_core::matrix_alloc(arena, 3, 3, &p_stop) == ErrorCode::Ok);
		// [0 1 0; 1 0 0; 0 0 1]
		matrix_core::matrix_fill_zero(p_stop);
		p_stop.at_mut(0, 1) = Rational::from_int(1);
		p_stop.at_mut(1, 0) = Rational::from_int(1);
		p_stop.at_mut(2, 2) = Rational::from_int(1);

		MatrixMutView p_full;
		assert(matrix_core::matrix_clone(arena, p_stop.view(), &p_full) == ErrorCode::Ok);

		Rational ignored{};
		std::size_t stop_ops = 0;
		matrix_core::RowOp last{};
		assert(matrix_core::detail::det_elim(p_stop, &ignored, &stop_ops, 1, &last) == ErrorCode::Ok);
		assert(stop_ops == 1);
		assert(last.kind == matrix_core::RowOpKind::Swap);

		// Full det.
		std::size_t full_ops = 0;
		assert(matrix_core::detail::det_elim(p_full, &det, &full_ops, static_cast<std::size_t>(-1), nullptr) == ErrorCode::Ok);
		assert(det.num() == -1 && det.den() == 1);

		// Error conditions.
		MatrixMutView ns;
		assert(matrix_core::matrix_alloc(arena, 2, 3, &ns) == ErrorCode::Ok);
		assert(matrix_core::detail::det_elim(ns, &det, nullptr, static_cast<std::size_t>(-1), nullptr) == ErrorCode::NotSquare);
		MatrixMutView bad_view{ 2, 2, 2, nullptr };
		assert(matrix_core::detail::det_elim(bad_view, &det, nullptr, static_cast<std::size_t>(-1), nullptr) == ErrorCode::Internal);
		assert(matrix_core::detail::det_elim(z, nullptr, nullptr, static_cast<std::size_t>(-1), nullptr) == ErrorCode::Internal);

		// row_op_caption branches.
		char buf[64];
		matrix_core::RowOp op;
		op.kind = matrix_core::RowOpKind::Swap;
		op.target_row = 0;
		op.source_row = 1;
		assert(matrix_core::row_op_caption(op, buf, sizeof(buf)) == ErrorCode::Ok);
		assert(std::strcmp(buf, "R1 <-> R2") == 0);

		op.kind = matrix_core::RowOpKind::Scale;
		op.target_row = 9; // R10 (two-digit)
		op.scalar = Rational::from_int(-2);
		assert(matrix_core::row_op_caption(op, buf, sizeof(buf)) == ErrorCode::Ok);
		assert(std::strstr(buf, "R10") != nullptr);

		op.kind = matrix_core::RowOpKind::AddMul;
		op.target_row = 1;
		op.source_row = 0;
		assert(Rational::make(1, 2, &op.scalar) == ErrorCode::Ok);
		assert(matrix_core::row_op_caption(op, buf, sizeof(buf)) == ErrorCode::Ok);
		assert(std::strstr(buf, "\\frac{1}{2}") != nullptr);

		assert(matrix_core::row_op_caption(op, nullptr, 0) == ErrorCode::BufferTooSmall);
		assert(matrix_core::row_op_caption(op, buf, 1) == ErrorCode::BufferTooSmall);
	}

	return 0;
}
