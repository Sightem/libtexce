#include "matrix_core/matrix_core.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>

using matrix_core::Arena;
using matrix_core::ArenaScope;
using matrix_core::ArenaScratchScope;

int main()
{
	{
		std::uint8_t buf[128];
		Arena arena(buf, sizeof(buf));

		void* p0 = arena.allocate(16, 1);
		assert(p0 != nullptr);
		const std::size_t used0 = arena.used();

		{
			ArenaScope scope(arena);
			void* p1 = arena.allocate(32, 1);
			assert(p1 != nullptr);
			assert(arena.used() > used0);
		}
		assert(arena.used() == used0);

		std::size_t used1 = 0;
		{
			ArenaScope scope(arena);
			void* p2 = arena.allocate(8, 1);
			assert(p2 != nullptr);
			used1 = arena.used();
			scope.commit();
		}
		assert(arena.used() == used1);
	}

	{
		std::uint8_t buf[64];
		Arena scratch(buf, sizeof(buf));

		void* p0 = scratch.allocate(8, 1);
		assert(p0 != nullptr);
		assert(scratch.used() > 0);

		ArenaScratchScope scope(scratch);
		assert(scratch.used() == 0);
	}

	return 0;
}

