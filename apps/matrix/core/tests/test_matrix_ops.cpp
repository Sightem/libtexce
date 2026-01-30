#include "matrix_core/matrix_core.hpp"

#include <cassert>
#include <cstring>
#include <limits>
#include <utility>

using matrix_core::ErrorCode;
using matrix_core::ExplainOptions;
using matrix_core::Arena;
using matrix_core::Explanation;
using matrix_core::MatrixMutView;
using matrix_core::MatrixView;
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
	assert(slab.init(128 * 1024) == ErrorCode::Ok);
	Arena persist(slab.data(), slab.size() / 2);
	Arena scratch(slab.data() + (slab.size() / 2), slab.size() - (slab.size() / 2));

	// matrix_view.cpp error branches.
	{
		std::uint8_t buf[64] = {};
		Arena tiny(buf, sizeof(buf));

		MatrixMutView m;
		assert(matrix_core::matrix_alloc(tiny, 0, 1, &m) == ErrorCode::InvalidDimension);
		assert(matrix_core::matrix_alloc(tiny, 1, 0, &m) == ErrorCode::InvalidDimension);
		assert(matrix_core::matrix_alloc(tiny, 7, 1, &m) == ErrorCode::InvalidDimension);
		assert(matrix_core::matrix_alloc(tiny, 1, 7, &m) == ErrorCode::InvalidDimension);
		assert(matrix_core::matrix_alloc(tiny, 1, 1, nullptr) == ErrorCode::Internal);

		Arena too_small(buf, 32);
		assert(matrix_core::matrix_alloc(too_small, 2, 2, &m) == ErrorCode::Overflow);

		MatrixMutView a = mat2(persist, 1, 2, 3, 4);
		assert(matrix_core::matrix_clone(persist, a.view(), nullptr) == ErrorCode::Internal);

		MatrixMutView a2;
		MatrixMutView b2;
		assert(matrix_core::matrix_alloc(persist, 2, 2, &a2) == ErrorCode::Ok);
		assert(matrix_core::matrix_alloc(persist, 1, 2, &b2) == ErrorCode::Ok);
		assert(matrix_core::matrix_copy(a2.view(), b2) == ErrorCode::DimensionMismatch);
		MatrixView bad_src{ 2, 2, 2, nullptr };
		assert(matrix_core::matrix_copy(bad_src, a2) == ErrorCode::Internal);

		matrix_core::matrix_fill_zero({ 2, 2, 2, nullptr });

		MatrixMutView mul_out;
		assert(matrix_core::matrix_alloc(persist, 2, 3, &mul_out) == ErrorCode::Ok);
		assert(matrix_core::matrix_mul(a2.view(), a2.view(), mul_out) == ErrorCode::DimensionMismatch);

		MatrixMutView add_out;
		assert(matrix_core::matrix_alloc(persist, 1, 2, &add_out) == ErrorCode::Ok);
		assert(matrix_core::matrix_add(a2.view(), a2.view(), add_out) == ErrorCode::DimensionMismatch);
	}

	// error.hpp helpers (exercise constexpr utilities via function pointers).
	{
		using Fn0 = matrix_core::Error (*)();
		Fn0 overflow_fn = &matrix_core::err_overflow;
		const matrix_core::Error e0 = overflow_fn();
		assert(e0.code == ErrorCode::Overflow);

		using Fn1 = matrix_core::Error (*)(matrix_core::Dim);
		Fn1 not_square_fn = &matrix_core::err_not_square;
		const matrix_core::Error e1 = not_square_fn({ 2, 3 });
		assert(e1.code == ErrorCode::NotSquare);

		Fn1 invalid_dim_fn = &matrix_core::err_invalid_dim;
		const matrix_core::Error e1b = invalid_dim_fn({ 0, 7 });
		assert(e1b.code == ErrorCode::InvalidDimension);

		using Fn2 = matrix_core::Error (*)(matrix_core::Dim, matrix_core::Dim);
		Fn2 mismatch_fn = &matrix_core::err_dim_mismatch;
		const matrix_core::Error e2 = mismatch_fn({ 1, 2 }, { 2, 1 });
		assert(e2.code == ErrorCode::DimensionMismatch);
	}

	// slab.hpp edge cases.
	{
		Slab s;
		assert(s.init(0) == ErrorCode::InvalidDimension);
		s.free();
		s.free();
	}

	// arena.hpp edge cases.
	{
		Arena a;
		assert(a.allocate(1, 1) == nullptr);

		std::uint8_t buf[32] = {};
		a.reset(buf, sizeof(buf));
		assert(a.allocate(0, 1) == nullptr);
		void* p = a.allocate(1, 0); // align==0 => align=1
		assert(p != nullptr);

		const std::size_t mark = a.mark();
		a.rewind(mark + 1); // no-op when mark > used
		assert(a.used() == mark);

		assert(a.allocate(std::numeric_limits<std::size_t>::max(), 1) == nullptr); // end<aligned overflow guard
	}

	{
		MatrixMutView a = mat2(persist, 1, 2, 3, 4);
		MatrixMutView b = mat2(persist, 5, 6, 7, 8);
		MatrixMutView c;
		assert(matrix_core::matrix_alloc(persist, 2, 2, &c) == ErrorCode::Ok);
		assert(matrix_core::matrix_add(a.view(), b.view(), c) == ErrorCode::Ok);
		assert(c.at(0, 0).num() == 6);
		assert(c.at(1, 1).num() == 12);
	}
	{
		MatrixMutView a = mat2(persist, 1, 2, 3, 4);
		MatrixMutView b = mat2(persist, 5, 6, 7, 8);
		MatrixMutView c;
		assert(matrix_core::matrix_alloc(persist, 2, 2, &c) == ErrorCode::Ok);
		assert(matrix_core::matrix_mul(a.view(), b.view(), c) == ErrorCode::Ok);
		// [1 2;3 4]*[5 6;7 8] = [19 22;43 50]
		assert(c.at(0, 0).num() == 19);
		assert(c.at(0, 1).num() == 22);
		assert(c.at(1, 0).num() == 43);
		assert(c.at(1, 1).num() == 50);
	}

	// Exercise ops_basic_view (op_add/sub/mul/transpose) + explanations.
	{
		MatrixMutView a = mat2(persist, 1, 2, 3, 4);
		MatrixMutView b = mat2(persist, 5, 6, 7, 8);
		MatrixMutView out;
		assert(matrix_core::matrix_alloc(persist, 2, 2, &out) == ErrorCode::Ok);

		// Dimension mismatch error reporting.
		{
			MatrixMutView bad;
			assert(matrix_core::matrix_alloc(persist, 1, 2, &bad) == ErrorCode::Ok);
			auto err = matrix_core::op_add(a.view(), bad.view(), out, nullptr, ExplainOptions{ .enable = false, .persist = nullptr });
			assert(!matrix_core::is_ok(err));
			assert(err.code == ErrorCode::DimensionMismatch);
			assert(err.a.rows == 2 && err.a.cols == 2);
			assert(err.b.rows == 1 && err.b.cols == 2);
		}

		// Explanation happy path.
		Explanation expl;
		auto err = matrix_core::op_add(a.view(), b.view(), out, &expl, ExplainOptions{ .enable = true, .persist = &persist });
		assert(matrix_core::is_ok(err));
		assert(expl.available());
		assert(expl.step_count() == 2);
		assert(out.at(0, 0).num() == 6);
		assert(out.at(1, 1).num() == 12);

		char caption[64];
		char latex[256];
		StepRenderBuffers bufs{ caption, sizeof(caption), latex, sizeof(latex), nullptr };

		assert(expl.render_step(0, bufs) == ErrorCode::Ok);
		assert(std::strcmp(latex, "C=A+B") == 0);
		assert(std::strcmp(caption, "") == 0);

		assert(expl.render_step(1, bufs) == ErrorCode::Ok);
		assert(std::strstr(latex, "\\begin{bmatrix}") != nullptr);

		assert(expl.render_step(2, bufs) == ErrorCode::StepOutOfRange);

		// Buffer-too-small paths.
		{
			char tiny[7];
			StepRenderBuffers tiny_bufs{ nullptr, 0, tiny, sizeof(tiny), nullptr };
			assert(expl.render_step(0, tiny_bufs) == ErrorCode::BufferTooSmall);
		}
		{
			StepRenderBuffers no_latex{ nullptr, 0, nullptr, 0, nullptr };
			assert(expl.render_step(0, no_latex) == ErrorCode::BufferTooSmall);
			assert(expl.render_step(1, no_latex) == ErrorCode::BufferTooSmall);
		}

		// Explanation move / self-move coverage.
		{
			Explanation moved = std::move(expl);
			assert(moved.available());
			assert(!expl.available());
			expl = std::move(moved);
			assert(expl.available());
			auto self_move = [](Explanation& e) -> Explanation&& { return std::move(e); };
			expl = self_move(expl);
			assert(expl.available());
		}

		// Explanation enable validation and allocation overflow.
		{
			auto err2 = matrix_core::op_add(a.view(), b.view(), out, nullptr, ExplainOptions{ .enable = true, .persist = &persist });
			assert(!matrix_core::is_ok(err2));
			assert(err2.code == ErrorCode::Internal);

			err2 = matrix_core::op_add(a.view(), b.view(), out, &expl, ExplainOptions{ .enable = true, .persist = nullptr });
			assert(!matrix_core::is_ok(err2));
			assert(err2.code == ErrorCode::Internal);

			std::uint8_t tiny_buf[8] = {};
			Arena tiny_persist(tiny_buf, sizeof(tiny_buf));
			err2 = matrix_core::op_add(a.view(), b.view(), out, &expl, ExplainOptions{ .enable = true, .persist = &tiny_persist });
			assert(!matrix_core::is_ok(err2));
			assert(err2.code == ErrorCode::Overflow);
		}
	}

	{
		MatrixMutView a = mat2(persist, 1, 2, 3, 4);
		MatrixMutView out;
		assert(matrix_core::matrix_alloc(persist, 2, 2, &out) == ErrorCode::Ok);

		// op_transpose dimension mismatch.
		{
			MatrixMutView bad_out;
			assert(matrix_core::matrix_alloc(persist, 2, 1, &bad_out) == ErrorCode::Ok);
			auto err = matrix_core::op_transpose(a.view(), bad_out, nullptr, ExplainOptions{ .enable = false, .persist = nullptr });
			assert(!matrix_core::is_ok(err));
			assert(err.code == ErrorCode::DimensionMismatch);
		}

		Explanation expl;
		auto err = matrix_core::op_transpose(a.view(), out, &expl, ExplainOptions{ .enable = true, .persist = &persist });
		assert(matrix_core::is_ok(err));
		assert(expl.available());
		assert(expl.step_count() == 2);
		assert(out.at(0, 1).num() == 3);
		assert(out.at(1, 0).num() == 2);

		char latex[256];
		StepRenderBuffers bufs{ nullptr, 0, latex, sizeof(latex), nullptr };
		assert(expl.render_step(0, bufs) == ErrorCode::Ok);
		assert(std::strstr(latex, "\\begin{bmatrix}") != nullptr);
		assert(expl.render_step(1, bufs) == ErrorCode::Ok);
		assert(expl.render_step(2, bufs) == ErrorCode::StepOutOfRange);

		StepRenderBuffers no_latex{ nullptr, 0, nullptr, 0, nullptr };
		assert(expl.render_step(0, no_latex) == ErrorCode::BufferTooSmall);
		assert(expl.render_step(1, no_latex) == ErrorCode::BufferTooSmall);
	}

	// Empty Explanation behavior.
	{
		Explanation empty;
		assert(!empty.available());
		assert(empty.step_count() == 0);
		char latex[8];
		StepRenderBuffers bufs{ nullptr, 0, latex, sizeof(latex), nullptr };
		assert(empty.render_step(0, bufs) == ErrorCode::Internal);
	}

	{
		MatrixMutView a = mat2(persist, 1, 2, 3, 4);
		MatrixMutView b = mat2(persist, 5, 6, 7, 8);
		MatrixMutView out;
		assert(matrix_core::matrix_alloc(persist, 2, 2, &out) == ErrorCode::Ok);

		Explanation expl;
		auto err = matrix_core::op_sub(a.view(), b.view(), out, &expl, ExplainOptions{ .enable = true, .persist = &persist });
		assert(matrix_core::is_ok(err));
		assert(expl.available());
		assert(out.at(0, 0).num() == -4);
		assert(out.at(1, 1).num() == -4);

		char latex[64];
		StepRenderBuffers bufs{ nullptr, 0, latex, sizeof(latex), nullptr };
		assert(expl.render_step(0, bufs) == ErrorCode::Ok);
		assert(std::strcmp(latex, "C=A-B") == 0);
	}

	{
		MatrixMutView a = mat2(persist, 1, 2, 3, 4);
		MatrixMutView b = mat2(persist, 5, 6, 7, 8);
		MatrixMutView out;
		assert(matrix_core::matrix_alloc(persist, 2, 2, &out) == ErrorCode::Ok);

		// op_mul dimension mismatch error reporting.
		{
			MatrixMutView bad;
			assert(matrix_core::matrix_alloc(persist, 1, 1, &bad) == ErrorCode::Ok);
			auto err = matrix_core::op_mul(a.view(), bad.view(), out, nullptr, ExplainOptions{ .enable = false, .persist = nullptr });
			assert(!matrix_core::is_ok(err));
			assert(err.code == ErrorCode::DimensionMismatch);
			assert(err.a.rows == 2 && err.a.cols == 2);
			assert(err.b.rows == 1 && err.b.cols == 1);
		}

		Explanation expl;
		auto err = matrix_core::op_mul(a.view(), b.view(), out, &expl, ExplainOptions{ .enable = true, .persist = &persist });
		assert(matrix_core::is_ok(err));
		assert(expl.available());
		assert(out.at(0, 0).num() == 19);
		assert(out.at(1, 1).num() == 50);

		char latex[64];
		StepRenderBuffers bufs{ nullptr, 0, latex, sizeof(latex), nullptr };
		assert(expl.render_step(0, bufs) == ErrorCode::Ok);
		assert(std::strcmp(latex, "C=A*B") == 0);
	}

	return 0;
}
