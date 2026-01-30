#include "matrix_core/ops.hpp"

#include "matrix_core/latex.hpp"
#include "matrix_core/row_ops.hpp"
#include "matrix_core/row_reduction.hpp"

#include <new>

namespace matrix_core
{
namespace
{

ErrorCode echelon_apply(MatrixMutView m, EchelonKind kind, OpObserver* obs) noexcept
{
	const std::uint8_t rows = m.rows;
	const std::uint8_t cols = m.cols;

	std::uint8_t pivot_row = 0;
	for (std::uint8_t pivot_col = 0; pivot_col < cols && pivot_row < rows; pivot_col++)
	{
		// Find pivot row.
		std::uint8_t best_row = pivot_row;
		bool found = false;
		for (std::uint8_t row = pivot_row; row < rows; row++)
		{
			if (!m.at(row, pivot_col).is_zero())
			{
				best_row = row;
				found = true;
				break;
			}
		}
		if (!found)
			continue;

		if (best_row != pivot_row)
		{
			apply_swap(m, pivot_row, best_row);
			if (obs)
			{
				RowOp op;
				op.kind = RowOpKind::Swap;
				op.target_row = pivot_row;
				op.source_row = best_row;
				if (!obs->on_op(op))
					return ErrorCode::Ok;
			}
		}

		// Make pivot = 1 for RREF.
		if (kind == EchelonKind::Rref)
		{
			Rational pivot = m.at(pivot_row, pivot_col);
			Rational inv;
			ErrorCode ec = rational_div(Rational::from_int(1), pivot, &inv);
			if (!is_ok(ec))
				return ec;
			ec = apply_scale(m, pivot_row, inv);
			if (!is_ok(ec))
				return ec;
			if (obs)
			{
				RowOp op;
				op.kind = RowOpKind::Scale;
				op.target_row = pivot_row;
				op.scalar = inv;
				if (!obs->on_op(op))
					return ErrorCode::Ok;
			}
		}

		// Eliminate.
		for (std::uint8_t row = 0; row < rows; row++)
		{
			if (row == pivot_row)
				continue;
			if (kind == EchelonKind::Ref && row < pivot_row)
				continue;

			Rational entry = m.at(row, pivot_col);
			if (entry.is_zero())
				continue;

			Rational factor;
			ErrorCode ec = rational_neg(entry, &factor);
			if (!is_ok(ec))
				return ec;

			if (kind == EchelonKind::Ref)
			{
				Rational pivot = m.at(pivot_row, pivot_col);
				ec = rational_div(factor, pivot, &factor);
				if (!is_ok(ec))
					return ec;
			}

			ec = apply_addmul(m, row, pivot_row, factor);
			if (!is_ok(ec))
				return ec;

			if (obs)
			{
				RowOp op;
				op.kind = RowOpKind::AddMul;
				op.target_row = row;
				op.source_row = pivot_row;
				op.scalar = factor;
				if (!obs->on_op(op))
					return ErrorCode::Ok;
			}
		}

		pivot_row++;
	}

	return ErrorCode::Ok;
}

struct EchelonCtx
{
	MatrixView input;
	EchelonKind kind = EchelonKind::Rref;
	std::size_t op_count = 0;
};

std::size_t echelon_step_count(const void* vctx) noexcept
{
	const auto* ctx = static_cast<const EchelonCtx*>(vctx);
	return ctx->op_count + 1;
}

ErrorCode echelon_render_step(const void* vctx, std::size_t index, const StepRenderBuffers& out) noexcept
{
	const auto* ctx = static_cast<const EchelonCtx*>(vctx);
	if (!ctx->input.data)
		return ErrorCode::Internal;
	if (!out.scratch)
		return ErrorCode::Internal;

	if (out.caption && out.caption_cap)
		out.caption[0] = '\0';
	if (out.latex && out.latex_cap)
		out.latex[0] = '\0';

	if (index == 0)
		return latex::write_matrix(ctx->input, latex::MatrixBrackets::BMatrix, { out.latex, out.latex_cap });

	MatrixMutView work;
	ErrorCode ec = matrix_clone(*out.scratch, ctx->input, &work);
	if (!is_ok(ec))
		return ec;

	OpObserver obs;
	obs.target = index; // stop after applying index ops
	ec = echelon_apply(work, ctx->kind, &obs);
	if (!is_ok(ec))
		return ec;
	if (obs.count < index)
		return ErrorCode::StepOutOfRange;

	if (out.caption)
	{
		ec = row_op_caption(obs.last_op, out.caption, out.caption_cap);
		if (!is_ok(ec))
			return ec;
	}

	return latex::write_matrix(work.view(), latex::MatrixBrackets::BMatrix, { out.latex, out.latex_cap });
}

constexpr ExplanationVTable kEchelonVTable = {
	.step_count = &echelon_step_count,
	.render_step = &echelon_render_step,
	.destroy = nullptr,
};

} // namespace

Error op_echelon(MatrixView a, EchelonKind kind, MatrixMutView out, Explanation* expl, const ExplainOptions& opts) noexcept
{
	Error err;
	if (out.rows != a.rows || out.cols != a.cols)
		return { ErrorCode::DimensionMismatch, a.dim(), out.dim() };

	ErrorCode ec = matrix_copy(a, out);
	if (!is_ok(ec))
		return { ec };

	OpObserver obs;
	obs.target = static_cast<std::size_t>(-1);
	ec = echelon_apply(out, kind, &obs);
	if (!is_ok(ec))
	{
		err.code = ec;
		err.a = a.dim();
		return err;
	}

	if (opts.enable)
	{
		if (!opts.persist || !expl)
			return { ErrorCode::Internal };

		ArenaScope tx(*opts.persist);
		void* mem = opts.persist->allocate(sizeof(EchelonCtx), alignof(EchelonCtx));
		if (!mem)
			return { ErrorCode::Overflow };
		auto* ctx = new (mem) EchelonCtx{};
		ctx->input = a;
		ctx->kind = kind;
		ctx->op_count = obs.count;
		*expl = Explanation::make(ctx, &kEchelonVTable);
		tx.commit();
	}

	return err;
}

} // namespace matrix_core
