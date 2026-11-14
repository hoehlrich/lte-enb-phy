#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>

#include "lte/common/constants.hpp"
#include "lte/common/resource_grid.hpp"
#include "lte/common/types.hpp"
#include "lte/ofdm/ofdm_demodulator.hpp"
#include "lte/ofdm/ofdm_modulator.hpp"
#include "lte/sim/awgn.hpp"

namespace lte::sim {

struct ErrorCount {
    std::uint64_t bits = 0;
    std::uint64_t errors = 0;
};

class ThinSliceLink {
public:
    explicit ThinSliceLink(std::uint64_t seed);
    ErrorCount run_subframe(float snr_db);

private:
    std::uniform_int_distribution<std::uint8_t> bit_dist;
    std::mt19937 bit_rng_;
    AwgnChannel channel_;

    OfdmModulator modulator_;
    OfdmDemodulator demodulator_;

    static constexpr std::size_t kBitsPerElement = 2; // QPSK
    static constexpr std::size_t kBitsPerSymbol = kNumSubcarriers * kBitsPerElement;
    static constexpr std::size_t kBitsPerSubframe = kSymbolsPerSubframe * kBitsPerSymbol;

    std::array<std::uint8_t, kBitsPerSubframe> tx_bits_{};
    std::array<std::uint8_t, kBitsPerSubframe> rx_bits_{};
    ResourceGrid tx_grid_;
    ResourceGrid rx_grid_;
    std::array<cf32, kSamplesPerSubframe> samples_{};
};

// Runs subframes until min_errors or max_bits, whichever comes first
ErrorCount measure_ber(ThinSliceLink &link, float snr_db, std::uint64_t min_errors,
                       std::uint64_t max_bits);

}  // namespace lte::sim
