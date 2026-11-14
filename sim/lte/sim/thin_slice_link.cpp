#include "thin_slice_link.hpp"

#include <cstddef>
#include <cstdint>
#include <random>
#include <span>

#include "lte/common/constants.hpp"
#include "lte/mod/qpsk.hpp"

lte::sim::ThinSliceLink::ThinSliceLink(std::uint64_t seed)
    : bit_dist(0, 1), bit_rng_(seed), channel_(1) {}

lte::sim::ErrorCount lte::sim::ThinSliceLink::run_subframe(float snr_db) {
    for (std::uint8_t &bit : tx_bits_) {
        bit = bit_dist(bit_rng_);
    }

    for (size_t l = 0; l < kSymbolsPerSubframe; ++l) {
        qpsk_map(std::span(tx_bits_).subspan(l * kBitsPerSymbol, kBitsPerSymbol),
                 tx_grid_.symbol(l));
    }

    modulator_.modulate(tx_grid_, samples_);

    channel_.apply(samples_, snr_db);

    demodulator_.demodulate(samples_, rx_grid_);

    for (size_t l = 0; l < kSymbolsPerSubframe; ++l) {
        qpsk_demap_hard(rx_grid_.symbol(l),
                        std::span(rx_bits_).subspan(l * kBitsPerSymbol, kBitsPerSymbol));
    }

    size_t errors = 0;
    for (size_t i = 0; i < kBitsPerSubframe; ++i) {
        if (tx_bits_[i] != rx_bits_[i]) errors++;
    }

    return ErrorCount{kBitsPerSubframe, errors};
}
