#include "matrix_core/ops.hpp"

#include "matrix_core/latex.hpp"
#include "matrix_core/row_ops.hpp"
#include "matrix_core/row_reduction.hpp"

#include <new>

namespace matrix_core
{
namespace
{

constexpr bool is_one(const Rational& r) noexcept
{
	return r.num() == 1 && r.den() == 1;
}

ErrorCode inverse_apply(MatrixMutView aug, std::uint8_t n, OpObserver* obs) noexcept
{
	// aug is n x (2n): [A | I] -> [I | A^{-1}]
	if (!aug.data)
		return ErrorCode::Internal;
	if (aug.rows != n || aug.cols != static_cast<std::uint8_t>(2u * n))
		return ErrorCode::Internal;

	for (std::uint8_t pivot = 0; pivot < n; pivot++)
	{
		// Find pivot row.
		std::uint8_t best_row = pivot;
		bool found = false;
		for (std::uint8_t row = pivot; row < n; row++)
		{
			if (!aug.at(row, pivot).is_zero())
			{
				best_row = row;
				found = true;
				break;
			}
		}
		if (!found)
			return ErrorCode::Singular;

		// Swap pivot row into place.
		if (best_row != pivot)
		{
			apply_swap(aug, pivot, best_row);
			if (obs)
			{
				RowOp op;
				op.kind = RowOpKind::Swap;
				op.target_row = pivot;
				op.source_row = best_row;
				if (!obs->on_op(op))
					return ErrorCode::Ok;
			}
		}

		// Scale pivot row to make pivot = 1.
		const Rational pivot_val = aug.at(pivot, pivot);
		if (!is_one(pivot_val))
		{
			Rational inv;
			ErrorCode ec = rational_div(Rational::from_int(1), pivot_val, &inv);
			if (!is_ok(ec))
				return ec;
			ec = apply_scale(aug, pivot, inv);
			if (!is_ok(ec))
				return ec;
			if (obs)
			{
				RowOp op;
				op.kind = RowOpKind::Scale;
				op.target_row = pivot;
				op.scalar = inv;
				if (!obs->on_op(op))
					return ErrorCode::Ok;
			}
		}

		// Eliminate this column for all other rows.
		for (std::uint8_t row = 0; row < n; row++)
		{
			if (row == pivot)
				continue;

			const Rational entry = aug.at(row, pivot);
			if (entry.is_zero())
				continue;

			Rational factor;
			ErrorCode ec = rational_neg(entry, &factor);
			if (!is_ok(ec))
				return ec;

			ec = apply_addmul(aug, row, pivot, factor);
			if (!is_ok(ec))
				return ec;

			if (obs)
			{
				RowOp op;
				op.kind = RowOpKind::AddMul;
				op.target_row = row;
				op.source_row = pivot;
				op.scalar = factor;
				if (!obs->on_op(op))
					return ErrorCode::Ok;
			}
		}
	}

	return ErrorCode::Ok;
}

ErrorCode build_augmented(MatrixView a, Arena& arena, MatrixMutView* out_aug) noexcept
{
	if (!out_aug)
		return ErrorCode::Internal;
	if (!a.data)
		return ErrorCode::Internal;
	if (a.rows != a.cols)
		return ErrorCode::NotSquare;

	const std::uint8_t n = a.rows;
	const std::uint8_t cols = static_cast<std::uint8_t>(2u * n);
	const std::size_t count = static_cast<std::size_t>(n) * static_cast<std::size_t>(cols);
	void* mem = arena.allocate(sizeof(Rational) * count, alignof(Rational));
	if (!mem)
		return ErrorCode::Overflow;

	auto* data = static_cast<Rational*>(mem);
	for (std::size_t i = 0; i < count; i++)
		data[i] = Rational::from_int(0);

	MatrixMutView aug;
	aug.rows = n;
	aug.cols = cols;
	aug.stride = cols;
	aug.data = data;

	for (std::uint8_t row = 0; row < n; row++)
	{
		for (std::uint8_t col = 0; col < n; col++)
			aug.at_mut(row, col) = a.at(row, col);
		aug.at_mut(row, static_cast<std::uint8_t>(n + row)) = Rational::from_int(1);
	}

	*out_aug = aug;
	return ErrorCode::Ok;
}

struct InverseCtx
{
	MatrixView input;
	std::size_t op_count = 0;
};

std::size_t inverse_step_count(const void* vctx) noexcept
{
	const auto* ctx = static_cast<const InverseCtx*>(vctx);
	return ctx->op_count + 1;
}

ErrorCode inverse_render_step(const void* vctx, std::size_t index, const StepRenderBuffers& out) noexcept
{
	const auto* ctx = static_cast<const InverseCtx*>(vctx);
	if (!ctx->input.data)
		return ErrorCode::Internal;
	if (!out.scratch)
		return ErrorCode::Internal;

	if (out.caption && out.caption_cap)
		out.caption[0] = '\0';
	if (out.latex && out.latex_cap)
		out.latex[0] = '\0';

	const std::size_t total = ctx->op_count + 1;
	if (index >= total)
		return ErrorCode::StepOutOfRange;

	MatrixMutView aug;
	ErrorCode ec = build_augmented(ctx->input, *out.scratch, &aug);
	if (!is_ok(ec))
		return ec;

	const std::uint8_t n = ctx->input.rows;
	const MatrixView left{ n, n, aug.stride, aug.data };
	const MatrixView right{ n, n, aug.stride, aug.data + n };

	if (index == 0)
		return latex::write_augmented_matrix(left, right, { out.latex, out.latex_cap });

	OpObserver obs;
	obs.target = index;
	ec = inverse_apply(aug, n, &obs);
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

	return latex::write_augmented_matrix(left, right, { out.latex, out.latex_cap });
}

constexpr ExplanationVTable kInverseVTable = {
	.step_count = &inverse_step_count,
	.render_step = &inverse_render_step,
	.destroy = nullptr,
};

} // namespace

Error op_inverse(MatrixView a, Arena& scratch, MatrixMutView out, Explanation* expl, const ExplainOptions& opts) noexcept
{
	Error err;
	if (a.rows != a.cols)
		return err_not_square(a.dim());
	if (out.rows != a.rows || out.cols != a.cols)
		return err_dim_mismatch(a.dim(), out.dim());

	ArenaScratchScope scratch_scope(scratch);
	MatrixMutView aug;
	ErrorCode ec = build_augmented(a, scratch, &aug);
	if (!is_ok(ec))
	{
		err.code = ec;
		err.a = a.dim();
		return err;
	}

	OpObserver obs;
	obs.target = static_cast<std::size_t>(-1);
	ec = inverse_apply(aug, a.rows, &obs);
	if (!is_ok(ec))
	{
		if (ec == ErrorCode::Singular)
			return err_singular(a.dim());
		err.code = ec;
		err.a = a.dim();
		return err;
	}

	const std::uint8_t n = a.rows;
	for (std::uint8_t row = 0; row < n; row++)
	{
		for (std::uint8_t col = 0; col < n; col++)
			out.at_mut(row, col) = aug.at(row, static_cast<std::uint8_t>(n + col));
	}

	if (opts.enable)
	{
		if (!opts.persist || !expl)
			return { ErrorCode::Internal };

		ArenaScope tx(*opts.persist);
		void* mem = opts.persist->allocate(sizeof(InverseCtx), alignof(InverseCtx));
		if (!mem)
			return err_overflow();
		auto* ctx = new (mem) InverseCtx{};
		ctx->input = a;
		ctx->op_count = obs.count;
		*expl = Explanation::make(ctx, &kInverseVTable);
		tx.commit();
	}

	return err;
}

} // namespace matrix_core
