#include "matrix_core/ops.hpp"

#include "matrix_core/latex.hpp"
#include "matrix_core/row_ops.hpp"
#include "matrix_core/writer.hpp"

#include "matrix_core/det_detail.hpp"

#include <new>

namespace matrix_core
{
namespace
{
ErrorCode build_submatrix(MatrixView a, std::uint8_t del_r, std::uint8_t del_c, Arena& arena, MatrixMutView* out) noexcept
{
	if (a.rows != a.cols)
		return ErrorCode::NotSquare;
	if (a.rows <= 1)
		return ErrorCode::InvalidDimension;
	if (del_r >= a.rows || del_c >= a.cols)
		return ErrorCode::IndexOutOfRange;

	MatrixMutView sub;
	ErrorCode ec = matrix_alloc(arena, static_cast<std::uint8_t>(a.rows - 1), static_cast<std::uint8_t>(a.cols - 1), &sub);
	if (!is_ok(ec))
		return ec;

	std::uint8_t dest_row = 0;
	for (std::uint8_t row = 0; row < a.rows; row++)
	{
		if (row == del_r)
			continue;
		std::uint8_t dest_col = 0;
		for (std::uint8_t col = 0; col < a.cols; col++)
		{
			if (col == del_c)
				continue;
			sub.at_mut(dest_row, dest_col) = a.at(row, col);
			dest_col++;
		}
		dest_row++;
	}

	*out = sub;
	return ErrorCode::Ok;
}

struct MinorCofactorCtx
{
	MatrixView a;
	std::uint8_t target_row = 0;
	std::uint8_t target_col = 0;
	MinorCofactorMode mode = MinorCofactorMode::Both;
	Rational minor = Rational::from_int(0);
	Rational cofactor = Rational::from_int(0);
	std::size_t op_count = 0;
};

std::size_t minor_cofactor_step_count(const void* vctx) noexcept
{
	const auto* ctx = static_cast<const MinorCofactorCtx*>(vctx);
	const std::uint8_t n = ctx->a.rows;

	std::size_t finals = 1;
	if (ctx->mode == MinorCofactorMode::Both)
		finals = 2;

	const std::size_t base = (n <= 1) ? 1 : 2;
	return base + ctx->op_count + finals;
}



ErrorCode write_minor_latex(const MinorCofactorCtx& ctx, char* out, std::size_t cap) noexcept
{
	if (!out || cap == 0)
		return ErrorCode::BufferTooSmall;

	Writer w{ out, cap, 0 };
	ErrorCode ec = w.append("M_{");
	if (!is_ok(ec))
		return ec;
	ec = w.append_index1(ctx.target_row);
	if (!is_ok(ec))
		return ec;
	ec = w.append(",");
	if (!is_ok(ec))
		return ec;
	ec = w.append_index1(ctx.target_col);
	if (!is_ok(ec))
		return ec;
	ec = w.append("} = \\det(A_{(");
	if (!is_ok(ec))
		return ec;
	ec = w.append_index1(ctx.target_row);
	if (!is_ok(ec))
		return ec;
	ec = w.append(",");
	if (!is_ok(ec))
		return ec;
	ec = w.append_index1(ctx.target_col);
	if (!is_ok(ec))
		return ec;
	ec = w.append(")}) = ");
	if (!is_ok(ec))
		return ec;
	return w.append_rational_latex(ctx.minor);
}

ErrorCode write_cofactor_latex(const MinorCofactorCtx& ctx, char* out, std::size_t cap) noexcept
{
	if (!out || cap == 0)
		return ErrorCode::BufferTooSmall;

	const std::uint32_t exp = static_cast<std::uint32_t>(ctx.target_row + 1u) + static_cast<std::uint32_t>(ctx.target_col + 1u);

	Writer w{ out, cap, 0 };
	ErrorCode ec = w.append("C_{");
	if (!is_ok(ec))
		return ec;
	ec = w.append_index1(ctx.target_row);
	if (!is_ok(ec))
		return ec;
	ec = w.append(",");
	if (!is_ok(ec))
		return ec;
	ec = w.append_index1(ctx.target_col);
	if (!is_ok(ec))
		return ec;
	ec = w.append("} = (-1)^{");
	if (!is_ok(ec))
		return ec;
	ec = w.append_u64(exp);
	if (!is_ok(ec))
		return ec;
	ec = w.append("} M_{");
	if (!is_ok(ec))
		return ec;
	ec = w.append_index1(ctx.target_row);
	if (!is_ok(ec))
		return ec;
	ec = w.append(",");
	if (!is_ok(ec))
		return ec;
	ec = w.append_index1(ctx.target_col);
	if (!is_ok(ec))
		return ec;
	ec = w.append("} = ");
	if (!is_ok(ec))
		return ec;
	return w.append_rational_latex(ctx.cofactor);
}

ErrorCode minor_cofactor_render_step(const void* vctx, std::size_t index, const StepRenderBuffers& out) noexcept
{
	const auto* ctx = static_cast<const MinorCofactorCtx*>(vctx);
	if (!ctx->a.data)
		return ErrorCode::Internal;
	if (!out.scratch)
		return ErrorCode::Internal;

	if (out.caption && out.caption_cap)
		out.caption[0] = '\0';
	if (out.latex && out.latex_cap)
		out.latex[0] = '\0';

	const std::uint8_t n = ctx->a.rows;
	const std::size_t finals = (ctx->mode == MinorCofactorMode::Both) ? 2 : 1;
	const std::size_t base = (n <= 1) ? 1 : 2;
	const std::size_t total = base + ctx->op_count + finals;
	if (index >= total)
		return ErrorCode::StepOutOfRange;

	if (index == 0)
		return latex::write_matrix(ctx->a, latex::MatrixBrackets::BMatrix, { out.latex, out.latex_cap });

	if (n <= 1)
	{
		const std::size_t formula_index = index - 1;
		if (ctx->mode == MinorCofactorMode::Both)
		{
			if (formula_index == 0)
				return write_minor_latex(*ctx, out.latex, out.latex_cap);
			return write_cofactor_latex(*ctx, out.latex, out.latex_cap);
		}
		if (ctx->mode == MinorCofactorMode::Minor)
			return write_minor_latex(*ctx, out.latex, out.latex_cap);
		return write_cofactor_latex(*ctx, out.latex, out.latex_cap);
	}

	MatrixMutView sub;
	ErrorCode ec = build_submatrix(ctx->a, ctx->target_row, ctx->target_col, *out.scratch, &sub);
	if (!is_ok(ec))
		return ec;

	if (index == 1)
	{
		if (out.caption && out.caption_cap)
		{
			Writer w{ out.caption, out.caption_cap, 0 };
			ErrorCode ec2 = w.append("Delete row ");
			if (!is_ok(ec2))
				return ec2;
			ec2 = w.append_index1(ctx->target_row);
			if (!is_ok(ec2))
				return ec2;
			ec2 = w.append(", col ");
			if (!is_ok(ec2))
				return ec2;
			ec2 = w.append_index1(ctx->target_col);
			if (!is_ok(ec2))
				return ec2;
		}

		return latex::write_matrix(sub.view(), latex::MatrixBrackets::BMatrix, { out.latex, out.latex_cap });
	}

	if (index < 1 + ctx->op_count + 1)
	{
		// index 2 -> after 1 op, index 3 -> after 2 ops, ...
		const std::size_t stop_after = index - 1;
		Rational ignored;
		RowOp last{};
		std::size_t ops = 0;
		ec = detail::det_elim(sub, &ignored, &ops, stop_after, &last);
		if (!is_ok(ec))
			return ec;
		if (ops < stop_after)
			return ErrorCode::StepOutOfRange;

		if (out.caption)
		{
			ec = row_op_caption(last, out.caption, out.caption_cap);
			if (!is_ok(ec))
				return ec;
		}
		return latex::write_matrix(sub.view(), latex::MatrixBrackets::BMatrix, { out.latex, out.latex_cap });
	}

	const std::size_t formula_index = index - (base + ctx->op_count);
	if (ctx->mode == MinorCofactorMode::Both)
	{
		if (formula_index == 0)
			return write_minor_latex(*ctx, out.latex, out.latex_cap);
		return write_cofactor_latex(*ctx, out.latex, out.latex_cap);
	}

	if (ctx->mode == MinorCofactorMode::Minor)
		return write_minor_latex(*ctx, out.latex, out.latex_cap);
	return write_cofactor_latex(*ctx, out.latex, out.latex_cap);
}

constexpr ExplanationVTable kMinorCofactorVTable = {
	.step_count = &minor_cofactor_step_count,
	.render_step = &minor_cofactor_render_step,
	.destroy = nullptr,
};

} // namespace

Error op_minor_cofactor(
	MatrixView a,
	std::uint8_t i,
	std::uint8_t j,
	MinorCofactorMode mode,
	Arena& scratch,
	Rational* minor_out,
	Rational* cofactor_out,
	Explanation* expl,
	const ExplainOptions& opts) noexcept
{
	Error err;
	if (a.rows != a.cols)
		return err_not_square(a.dim());
	if (i >= a.rows || j >= a.cols)
	{
		err.code = ErrorCode::IndexOutOfRange;
		err.a = a.dim();
		err.i = i;
		err.j = j;
		return err;
	}

	const bool need_minor = (mode == MinorCofactorMode::Minor || mode == MinorCofactorMode::Both);
	const bool need_cofactor = (mode == MinorCofactorMode::Cofactor || mode == MinorCofactorMode::Both);
	if (need_minor && !minor_out)
		return { ErrorCode::Internal };
	if (need_cofactor && !cofactor_out)
		return { ErrorCode::Internal };

	ArenaScratchScope scratch_scope(scratch);

	Rational minor = Rational::from_int(0);
	std::size_t op_count = 0;

	if (a.rows == 1)
	{
		minor = Rational::from_int(1);
		op_count = 0;
	}
	else
	{
		MatrixMutView sub;
		ErrorCode ec = build_submatrix(a, i, j, scratch, &sub);
		if (!is_ok(ec))
		{
			err.code = ec;
			err.a = a.dim();
			return err;
		}

		ec = detail::det_elim(sub, &minor, &op_count, static_cast<std::size_t>(-1), nullptr);
		if (!is_ok(ec))
		{
			err.code = ec;
			err.a = a.dim();
			return err;
		}
	}

	Rational cofactor = minor;
	if (((i + j) & 1u) != 0u)
	{
		ErrorCode ec = rational_neg(minor, &cofactor);
		if (!is_ok(ec))
		{
			err.code = ec;
			err.a = a.dim();
			return err;
		}
	}

	if (need_minor)
		*minor_out = minor;
	if (need_cofactor)
		*cofactor_out = cofactor;

	if (opts.enable)
	{
		if (!opts.persist || !expl)
			return { ErrorCode::Internal };
		ArenaScope tx(*opts.persist);
		void* mem = opts.persist->allocate(sizeof(MinorCofactorCtx), alignof(MinorCofactorCtx));
		if (!mem)
			return err_overflow();
		auto* ctx = new (mem) MinorCofactorCtx{};
		ctx->a = a;
		ctx->target_row = i;
		ctx->target_col = j;
		ctx->mode = mode;
		ctx->minor = minor;
		ctx->cofactor = cofactor;
		ctx->op_count = op_count;
		*expl = Explanation::make(ctx, &kMinorCofactorVTable);
		tx.commit();
	}

	return err;
}

} // namespace matrix_core
