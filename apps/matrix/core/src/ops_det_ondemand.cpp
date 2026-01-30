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
struct DetCtx
{
	MatrixView input;
	Rational det = Rational::from_int(0);
	std::size_t op_count = 0;
};

std::size_t det_step_count(const void* vctx) noexcept
{
	const auto* ctx = static_cast<const DetCtx*>(vctx);
	// 0: start matrix, 1..op_count: after each row op, last: det value
	return ctx->op_count + 2;
}

ErrorCode det_render_step(const void* vctx, std::size_t index, const StepRenderBuffers& out) noexcept
{
	const auto* ctx = static_cast<const DetCtx*>(vctx);
	if (!ctx->input.data)
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

	if (index == 0)
		return latex::write_matrix(ctx->input, latex::MatrixBrackets::VMatrix, { out.latex, out.latex_cap });

	if (index == total - 1)
	{
		Writer w{ out.latex, out.latex_cap, 0 };
		ErrorCode ec = w.append("\\det(A) = ");
		if (!is_ok(ec))
			return ec;
		return w.append_rational_latex(ctx->det);
	}

	MatrixMutView work;
	ErrorCode ec = matrix_clone(*out.scratch, ctx->input, &work);
	if (!is_ok(ec))
		return ec;

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

constexpr ExplanationVTable kDetVTable = {
	.step_count = &det_step_count,
	.render_step = &det_render_step,
	.destroy = nullptr,
};

} // namespace

Error op_det(MatrixView a, Arena& scratch, Rational* out, Explanation* expl, const ExplainOptions& opts) noexcept
{
	Error err;
	if (!out)
		return { ErrorCode::Internal };
	if (a.rows != a.cols)
		return err_not_square(a.dim());

	ArenaScratchScope scratch_scope(scratch);
	MatrixMutView work;
	ErrorCode ec = matrix_clone(scratch, a, &work);
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
		void* mem = opts.persist->allocate(sizeof(DetCtx), alignof(DetCtx));
		if (!mem)
			return { ErrorCode::Overflow };
		auto* ctx = new (mem) DetCtx{};
		ctx->input = a;
		ctx->det = det;
		ctx->op_count = op_count;
		*expl = Explanation::make(ctx, &kDetVTable);
		tx.commit();
	}

	return err;
}

} // namespace matrix_core
