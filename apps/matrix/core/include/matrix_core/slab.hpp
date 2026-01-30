#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include "matrix_core/error.hpp"

namespace matrix_core
{
class Slab
{
public:
	Slab() noexcept = default;
	Slab(const Slab&) = delete;
	Slab& operator=(const Slab&) = delete;

	~Slab() { free(); }

	ErrorCode init(std::size_t bytes) noexcept
	{
		free();
		if (bytes == 0)
			return ErrorCode::InvalidDimension;
		void* mem = std::malloc(bytes);
		if (!mem)
			return ErrorCode::Overflow;
		data_ = static_cast<std::uint8_t*>(mem);
		size_ = bytes;
		return ErrorCode::Ok;
	}

	void free() noexcept
	{
		if (data_)
			std::free(data_);
		data_ = nullptr;
		size_ = 0;
	}

	std::uint8_t* data() noexcept { return data_; }
	const std::uint8_t* data() const noexcept { return data_; }
	std::size_t size() const noexcept { return size_; }

private:
	std::uint8_t* data_ = nullptr;
	std::size_t size_ = 0;
};
} // namespace matrix_core

