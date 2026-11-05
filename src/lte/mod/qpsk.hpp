#pragma once

#include <cstdint>
#include <span>

#include "lte/common/types.hpp"

namespace lte {

// QPSK modulation mapper, 36.211 §7.1.2
// Precondition: bits.size() == 2 * out.size(), and every bit is 0 or 1
void qpsk_map(std::span<const std::uint8_t> bits, std::span<lte::cf32> out);

// QPSK hard demodulator, 36.211 §7.1.2
// Precondition: bits.size() == 2 * symbols.size()
void qpsk_demap_hard(std::span<const cf32> symbols, std::span<std::uint8_t> bits);

}  // namespace lte
