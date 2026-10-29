#include "lte/mod/qpsk.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>

#include "lte/common/contract.hpp"
#include "lte/common/types.hpp"

// QPSK modulation mapper, 36.211 §7.1.2
// Precondition: bits.size() == 2 * out.size(), and every bit is 0 or 1
void lte::qpsk_map(std::span<const std::uint8_t> bits, std::span<lte::cf32> out) {
    LTE_EXPECTS(bits.size() == 2 * out.size());
    const float magnitude = 1 / std::sqrt(2.0f);
    for (std::size_t i = 0; i < out.size(); ++i) {
        LTE_EXPECTS(bits[i * 2] <= 1 && bits[i * 2 + 1] <= 1);

        out[i] = {bits[i * 2] ? -magnitude : magnitude, bits[i * 2 + 1] ? -magnitude : magnitude};
    }
}
