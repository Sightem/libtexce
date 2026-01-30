#pragma once

#include <cstddef>
#include <cstdint>

namespace matrix_core
{
constexpr std::uint8_t kMaxRows = 6;
constexpr std::uint8_t kMaxCols = 6;
constexpr std::size_t kMaxEntries = static_cast<std::size_t>(kMaxRows) * static_cast<std::size_t>(kMaxCols);
} // namespace matrix_core

