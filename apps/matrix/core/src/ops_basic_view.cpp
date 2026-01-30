#include "matrix_core/ops.hpp"

#include "matrix_core/latex.hpp"

#include <new>

namespace matrix_core
{
namespace
{
struct BinaryCtx
{
	MatrixView a;
	MatrixView b;
	MatrixView result;
	char op = '+';
};

std::size_t binary_step_count(const void*) noexcept
{
	// 0: formula, 1: result matrix
	return 2;
}

ErrorCode binary_render_step(const void* vctx, std::size_t index, const StepRenderBuffers& out) noexcept
{
	const auto* ctx = static_cast<const BinaryCtx*>(vctx);
	if (index >= 2)
		return ErrorCode::StepOutOfRange;

	if (out.caption && out.caption_cap)
		out.caption[0] = '\0';
	if (out.latex && out.latex_cap)
		out.latex[0] = '\0';

	if (index == 0)
	{
		if (!out.latex || out.latex_cap < 8)
			return ErrorCode::BufferTooSmall;
		out.latex[0] = 'C';
		out.latex[1] = '=';
		out.latex[2] = 'A';
		out.latex[3] = ctx->op;
		out.latex[4] = 'B';
		out.latex[5] = '\0';
		return ErrorCode::Ok;
	}

	return latex::write_matrix(ctx->result, latex::MatrixBrackets::BMatrix, { out.latex, out.latex_cap });
}

constexpr ExplanationVTable kBinaryVTable = {
	.step_count = &binary_step_count,
	.render_step = &binary_render_step,
	.destroy = nullptr,
};

struct UnaryCtx
{
	MatrixView a;
	MatrixView result;
};

std::size_t unary_step_count(const void*) noexcept
{
	// 0: input matrix, 1: result matrix
	return 2;
}

ErrorCode unary_render_step(const void* vctx, std::size_t index, const StepRenderBuffers& out) noexcept
{
	const auto* ctx = static_cast<const UnaryCtx*>(vctx);
	if (index >= 2)
		return ErrorCode::StepOutOfRange;

	if (out.caption && out.caption_cap)
		out.caption[0] = '\0';
	if (out.latex && out.latex_cap)
		out.latex[0] = '\0';

	if (index == 0)
		return latex::write_matrix(ctx->a, latex::MatrixBrackets::BMatrix, { out.latex, out.latex_cap });
	return latex::write_matrix(ctx->result, latex::MatrixBrackets::BMatrix, { out.latex, out.latex_cap });
}

constexpr ExplanationVTable kUnaryVTable = {
	.step_count = &unary_step_count,
	.render_step = &unary_render_step,
	.destroy = nullptr,
};

template <typename Ctx>
ErrorCode ctx_alloc(Arena& arena, Ctx** out) noexcept
{
	void* mem = arena.allocate(sizeof(Ctx), alignof(Ctx));
	if (!mem)
		return ErrorCode::Overflow;
	*out = new (mem) Ctx{};
	return ErrorCode::Ok;
}

} // namespace

Error op_add(MatrixView a, MatrixView b, MatrixMutView out, Explanation* expl, const ExplainOptions& opts) noexcept
{
	Error err;
	ErrorCode ec = matrix_add(a, b, out);
	if (!is_ok(ec))
	{
		err.code = ec;
		err.a = a.dim();
		err.b = b.dim();
		return err;
	}

	if (opts.enable)
	{
		if (!opts.persist || !expl)
			return { ErrorCode::Internal };

		ArenaScope tx(*opts.persist);
		BinaryCtx* ctx = nullptr;
		ec = ctx_alloc(*opts.persist, &ctx);
		if (!is_ok(ec))
			return { ec };
		ctx->a = a;
		ctx->b = b;
		ctx->result = out.view();
		ctx->op = '+';
		*expl = Explanation::make(ctx, &kBinaryVTable);
		tx.commit();
	}

	return err;
}

Error op_sub(MatrixView a, MatrixView b, MatrixMutView out, Explanation* expl, const ExplainOptions& opts) noexcept
{
	Error err;
	ErrorCode ec = matrix_sub(a, b, out);
	if (!is_ok(ec))
	{
		err.code = ec;
		err.a = a.dim();
		err.b = b.dim();
		return err;
	}

	if (opts.enable)
	{
		if (!opts.persist || !expl)
			return { ErrorCode::Internal };

		ArenaScope tx(*opts.persist);
		BinaryCtx* ctx = nullptr;
		ec = ctx_alloc(*opts.persist, &ctx);
		if (!is_ok(ec))
			return { ec };
		ctx->a = a;
		ctx->b = b;
		ctx->result = out.view();
		ctx->op = '-';
		*expl = Explanation::make(ctx, &kBinaryVTable);
		tx.commit();
	}

	return err;
}

Error op_mul(MatrixView a, MatrixView b, MatrixMutView out, Explanation* expl, const ExplainOptions& opts) noexcept
{
	Error err;
	ErrorCode ec = matrix_mul(a, b, out);
	if (!is_ok(ec))
	{
		err.code = ec;
		err.a = a.dim();
		err.b = b.dim();
		return err;
	}

	if (opts.enable)
	{
		if (!opts.persist || !expl)
			return { ErrorCode::Internal };

		ArenaScope tx(*opts.persist);
		BinaryCtx* ctx = nullptr;
		ec = ctx_alloc(*opts.persist, &ctx);
		if (!is_ok(ec))
			return { ec };
		ctx->a = a;
		ctx->b = b;
		ctx->result = out.view();
		ctx->op = '*';
		*expl = Explanation::make(ctx, &kBinaryVTable);
		tx.commit();
	}

	return err;
}

Error op_transpose(MatrixView a, MatrixMutView out, Explanation* expl, const ExplainOptions& opts) noexcept
{
	Error err;
	ErrorCode ec = matrix_transpose(a, out);
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
		UnaryCtx* ctx = nullptr;
		ec = ctx_alloc(*opts.persist, &ctx);
		if (!is_ok(ec))
			return { ec };
		ctx->a = a;
		ctx->result = out.view();
		*expl = Explanation::make(ctx, &kUnaryVTable);
		tx.commit();
	}

	return err;
}

} // namespace matrix_core
