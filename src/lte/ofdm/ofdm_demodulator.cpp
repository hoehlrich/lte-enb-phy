#include "lte/ofdm/ofdm_demodulator.hpp"

#include <algorithm>
#include <cstddef>
#include <span>

#include "lte/common/constants.hpp"
#include "lte/common/contract.hpp"
#include "lte/common/fft.hpp"
#include "lte/common/resource_grid.hpp"
#include "lte/common/types.hpp"

lte::OfdmDemodulator::OfdmDemodulator() : fft_(kFftSize, Fft::Direction::kForward) {}

// DC (bin 0) and the guard bins (37-91) are never read from as they don't map to the resource grid
void lte::OfdmDemodulator::demodulate(std::span<const cf32> samples, ResourceGrid &grid) {
    LTE_EXPECTS(samples.size() == kSamplesPerSubframe);

    // Demodulate symbols
    std::size_t symbol_start = 0;
    for (std::size_t l = 0; l < kSymbolsPerSubframe; ++l) {
        // Ignore cyclic prefix and transform time domain samples into frequency bins
        const std::size_t cp = cp_length(l);
        std::ranges::copy(samples.subspan(symbol_start + cp, kFftSize),
                          fft_.in().begin());
        fft_.execute();

        // Map symbols from FFT bins to their respective resource elements
        const std::span<cf32> sym = grid.symbol(l);
        std::ranges::copy(fft_.out().subspan(kFftSize - kHalfSubcarriers), sym.begin());
        std::ranges::copy(fft_.out().subspan(1, kHalfSubcarriers),
                          sym.subspan(kHalfSubcarriers).begin());
        symbol_start += kFftSize + cp;
    }
}
