#include "lte/mod/qpsk.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>

#include "lte/common/contract.hpp"
#include "lte/common/types.hpp"

void lte::qpsk_map(std::span<const std::uint8_t> bits, std::span<lte::cf32> out) {
    LTE_EXPECTS(bits.size() == 2 * out.size());
    const float magnitude = 1 / std::sqrt(2.0f);
    for (std::size_t i = 0; i < out.size(); ++i) {
        LTE_EXPECTS(bits[i * 2] <= 1 && bits[i * 2 + 1] <= 1);

        out[i] = {bits[i * 2] ? -magnitude : magnitude, bits[i * 2 + 1] ? -magnitude : magnitude};
    }
}

void lte::qpsk_demap_hard(std::span<const cf32> symbols, std::span<std::uint8_t> bits) {
    LTE_EXPECTS(bits.size() == 2 * symbols.size());
    for (std::size_t i = 0; i < symbols.size(); ++i) {
        bits[i * 2] = symbols[i].real() < 0;
        bits[i * 2 + 1] = symbols[i].imag() < 0;
    }
}
