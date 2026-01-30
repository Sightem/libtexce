#include "matrix_core/explanation.hpp"

#include <utility>

namespace matrix_core
{
Explanation::Explanation(Explanation&& other) noexcept
{
	ctx_ = other.ctx_;
	vtable_ = other.vtable_;
	other.ctx_ = nullptr;
	other.vtable_ = nullptr;
}

Explanation& Explanation::operator=(Explanation&& other) noexcept
{
	if (this == &other)
		return *this;
	if (vtable_ && ctx_ && vtable_->destroy)
		vtable_->destroy(ctx_);
	ctx_ = other.ctx_;
	vtable_ = other.vtable_;
	other.ctx_ = nullptr;
	other.vtable_ = nullptr;
	return *this;
}

Explanation::~Explanation()
{
	if (vtable_ && ctx_ && vtable_->destroy)
		vtable_->destroy(ctx_);
	ctx_ = nullptr;
	vtable_ = nullptr;
}

std::size_t Explanation::step_count() const noexcept
{
	if (!available())
		return 0;
	return vtable_->step_count(ctx_);
}

ErrorCode Explanation::render_step(std::size_t index, const StepRenderBuffers& out) const noexcept
{
	if (!available())
		return ErrorCode::Internal;
	if (out.scratch)
	{
		ArenaScratchScope scope(*out.scratch);
		return vtable_->render_step(ctx_, index, out);
	}
	return vtable_->render_step(ctx_, index, out);
}

Explanation Explanation::make(void* ctx, const ExplanationVTable* vtable) noexcept
{
	Explanation e;
	e.ctx_ = ctx;
	e.vtable_ = vtable;
	return e;
}

} // namespace matrix_core
