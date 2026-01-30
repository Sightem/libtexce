#pragma once

#include <cstddef>
#include <cstdint>

#include "matrix_core/arena.hpp"
#include "matrix_core/error.hpp"

namespace matrix_core
{
struct StepRenderBuffers
{
	char* caption = nullptr;
	std::size_t caption_cap = 0;
	char* latex = nullptr;
	std::size_t latex_cap = 0;
	Arena* scratch = nullptr;
};

struct ExplanationVTable;

class Explanation
{
public:
	Explanation() noexcept = default;
	Explanation(const Explanation&) = delete;
	Explanation& operator=(const Explanation&) = delete;

	Explanation(Explanation&& other) noexcept;
	Explanation& operator=(Explanation&& other) noexcept;

	~Explanation();

	bool available() const noexcept { return vtable_ != nullptr && ctx_ != nullptr; }

	std::size_t step_count() const noexcept;
	ErrorCode render_step(In std::size_t index, In const StepRenderBuffers& out) const noexcept;

	static Explanation make(void* ctx, const ExplanationVTable* vtable) noexcept;

private:
	void* ctx_ = nullptr;
	const ExplanationVTable* vtable_ = nullptr;
};

struct ExplanationVTable
{
	std::size_t (*step_count)(In const void*) noexcept;
	ErrorCode (*render_step)(In const void*, In std::size_t, In const StepRenderBuffers&) noexcept;
	void (*destroy)(InOut void*) noexcept;
};

} // namespace matrix_core
