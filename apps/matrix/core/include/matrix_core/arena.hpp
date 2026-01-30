#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace matrix_core
{
class Arena
{
public:
	Arena() noexcept = default;
	Arena(void* buffer, std::size_t capacity) noexcept { reset(buffer, capacity); }

	void reset(void* buffer, std::size_t capacity) noexcept
	{
		base_ = static_cast<std::uint8_t*>(buffer);
		cap_ = capacity;
		used_ = 0;
	}

	void clear() noexcept { used_ = 0; }

	std::size_t used() const noexcept { return used_; }
	std::size_t capacity() const noexcept { return cap_; }

	std::size_t mark() const noexcept { return used_; }

	void rewind(std::size_t mark) noexcept
	{
		if (mark <= used_)
			used_ = mark;
	}

	void* allocate(std::size_t size, std::size_t align) noexcept
	{
		if (!base_ || cap_ == 0 || size == 0)
			return nullptr;
		if (align == 0)
			align = 1;

		const std::uintptr_t base_addr = reinterpret_cast<std::uintptr_t>(base_);
		const std::uintptr_t cur = base_addr + used_;
		const std::uintptr_t aligned = (cur + (align - 1u)) & ~(align - 1u);
		const std::uintptr_t end = aligned + size;
		if (end < aligned)
			return nullptr;

		const std::size_t new_used = static_cast<std::size_t>(end - base_addr);
		if (new_used > cap_)
			return nullptr;

		used_ = new_used;
		return reinterpret_cast<void*>(aligned);
	}

private:
	std::uint8_t* base_ = nullptr;
	std::size_t cap_ = 0;
	std::size_t used_ = 0;
};

// captures arena mark() and rewinds on destruction unless commit() is called
// useful for transactional allocations in persist arenas
class ArenaScope final
{
public:
	explicit ArenaScope(Arena& arena) noexcept
		: arena_(arena)
		, mark_(arena.mark())
	{
	}

	ArenaScope(const ArenaScope&) = delete;
	ArenaScope& operator=(const ArenaScope&) = delete;

	~ArenaScope() noexcept
	{
		if (!active_)
			return;
#ifndef NDEBUG
		// if the arena was rewound past our mark, the scope is being used out of
		// order (or the arena was manually rewound). this would defeat the
		// intended transactional semantics
		assert(arena_.used() >= mark_);
#endif
		arena_.rewind(mark_);
	}

	void commit() noexcept { active_ = false; }

private:
	Arena& arena_;
	std::size_t mark_ = 0;
	bool active_ = true;
};

// clears the arena on entry. this prevents scratch allocations
// from persisting across calls even if a caller forgets to clear manually
class ArenaScratchScope final
{
public:
	explicit ArenaScratchScope(Arena& arena) noexcept
		: arena_(arena)
	{
		arena_.clear();
	}

	ArenaScratchScope(const ArenaScratchScope&) = delete;
	ArenaScratchScope& operator=(const ArenaScratchScope&) = delete;

	~ArenaScratchScope() noexcept = default;

private:
	Arena& arena_;
};
} // namespace matrix_core
