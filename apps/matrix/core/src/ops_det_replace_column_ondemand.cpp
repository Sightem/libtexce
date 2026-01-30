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
struct DetReplaceColCtx
{
	MatrixView a;
	MatrixView b;
	std::uint8_t col = 0;
	Rational det = Rational::from_int(0);
	std::size_t op_count = 0;
};

std::size_t det_replace_step_count(const void* vctx) noexcept
{
	const auto* ctx = static_cast<const DetReplaceColCtx*>(vctx);
	return ctx->op_count + 2;
}

ErrorCode build_matrix(MatrixView a, MatrixView b, std::uint8_t col, Arena& arena, MatrixMutView* out) noexcept
{
	MatrixMutView work;
	ErrorCode ec = matrix_clone(arena, a, &work);
	if (!is_ok(ec))
		return ec;

	for (std::uint8_t row = 0; row < a.rows; row++)
		work.at_mut(row, col) = b.at(row, 0);

	*out = work;
	return ErrorCode::Ok;
}

ErrorCode det_replace_render_step(const void* vctx, std::size_t index, const StepRenderBuffers& out) noexcept
{
	const auto* ctx = static_cast<const DetReplaceColCtx*>(vctx);
	if (!ctx->a.data || !ctx->b.data)
		return ErrorCode::Internal;
	if (!out.scratch)
		return ErrorCode::Internal;

	if (out.caption && out.caption_cap)
		out.caption[0] = '\0';
	if (out.latex && out.latex_cap)
		out.latex[0] = '\0';

	const std::size_t total = ctx->op_count + 2;
	if (index >= total)
		return ErrorCode::StepOutOfRange;

	MatrixMutView work;
	ErrorCode ec = build_matrix(ctx->a, ctx->b, ctx->col, *out.scratch, &work);
	if (!is_ok(ec))
		return ec;

	if (index == 0)
		return latex::write_matrix(work.view(), latex::MatrixBrackets::VMatrix, { out.latex, out.latex_cap });

	if (index == total - 1)
	{
		Writer w{ out.latex, out.latex_cap, 0 };
		ErrorCode ec2 = w.append("\\det(A_{");
		if (!is_ok(ec2))
			return ec2;
		ec2 = w.append_index1(ctx->col);
		if (!is_ok(ec2))
			return ec2;
		ec2 = w.append("}) = ");
		if (!is_ok(ec2))
			return ec2;
		return w.append_rational_latex(ctx->det);
	}

	Rational ignored;
	RowOp last{};
	std::size_t ops = 0;
	ec = detail::det_elim(work, &ignored, &ops, index, &last);
	if (!is_ok(ec))
		return ec;
	if (ops < index)
		return ErrorCode::StepOutOfRange;

	if (out.caption)
	{
		ec = row_op_caption(last, out.caption, out.caption_cap);
		if (!is_ok(ec))
			return ec;
	}

	return latex::write_matrix(work.view(), latex::MatrixBrackets::VMatrix, { out.latex, out.latex_cap });
}

constexpr ExplanationVTable kDetReplaceColVTable = {
	.step_count = &det_replace_step_count,
	.render_step = &det_replace_render_step,
	.destroy = nullptr,
};

} // namespace

Error op_det_replace_column(MatrixView a, MatrixView b, std::uint8_t col, Arena& scratch, Rational* out, Explanation* expl, const ExplainOptions& opts) noexcept
{
	Error err;
	if (!out)
		return { ErrorCode::Internal };
	if (a.rows != a.cols)
		return err_not_square(a.dim());
	if (b.rows != a.rows || b.cols != 1)
		return err_dim_mismatch(a.dim(), b.dim());
	if (col >= a.cols)
	{
		err.code = ErrorCode::IndexOutOfRange;
		err.a = a.dim();
		err.i = col;
		return err;
	}

	ArenaScratchScope scratch_scope(scratch);
	MatrixMutView work;
	ErrorCode ec = build_matrix(a, b, col, scratch, &work);
	if (!is_ok(ec))
		return { ec };

	Rational det;
	std::size_t op_count = 0;
	ec = detail::det_elim(work, &det, &op_count, static_cast<std::size_t>(-1), nullptr);
	if (!is_ok(ec))
	{
		err.code = ec;
		err.a = a.dim();
		return err;
	}

	*out = det;

	if (opts.enable)
	{
		if (!opts.persist || !expl)
			return { ErrorCode::Internal };
		ArenaScope tx(*opts.persist);
		void* mem = opts.persist->allocate(sizeof(DetReplaceColCtx), alignof(DetReplaceColCtx));
		if (!mem)
			return { ErrorCode::Overflow };
		auto* ctx = new (mem) DetReplaceColCtx{};
		ctx->a = a;
		ctx->b = b;
		ctx->col = col;
		ctx->det = det;
		ctx->op_count = op_count;
		*expl = Explanation::make(ctx, &kDetReplaceColVTable);
		tx.commit();
	}

	return err;
}

} // namespace matrix_core
