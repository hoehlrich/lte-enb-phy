#pragma once

#include <cstddef>

namespace lte {

// 36.211 Table 6.2.3-1 (normal CP), 1.4 MHz

inline constexpr std::size_t kNumRb = 6;              // N_RB
inline constexpr std::size_t kSubcarriersPerRb = 12;  // N_sc^RB
inline constexpr std::size_t kNumSubcarriers = kNumRb * kSubcarriersPerRb;
inline constexpr std::size_t kSymbolsPerSlot = 7;  // N_symb, normal CP
inline constexpr std::size_t kSymbolsPerSubframe = 2 * kSymbolsPerSlot;

}  // namespace lte
