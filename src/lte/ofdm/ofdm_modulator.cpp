#include "lte/ofdm/ofdm_modulator.hpp"

#include <algorithm>
#include <cstddef>
#include <span>

#include "lte/common/constants.hpp"
#include "lte/common/contract.hpp"
#include "lte/common/fft.hpp"
#include "lte/common/resource_grid.hpp"
#include "lte/common/types.hpp"

namespace {
// lteOFDMModulate uses MATLAB's ifft convention (1/N); FFTW's inverse is unnormalized.
// Measured: raw output / Toolbox output = 128.000.
constexpr float kIfftScale = 1.0f / lte::kFftSize;
}  // namespace

lte::OfdmModulator::OfdmModulator() : ifft_(kFftSize, Fft::Direction::kInverse) {}

// DC (bin 0) and the guard bins (37-91) are never to be written, must stay 0
void lte::OfdmModulator::modulate(const ResourceGrid &grid, std::span<cf32> out) {
    LTE_EXPECTS(out.size() == kSamplesPerSubframe);

    // Modulate symbols
    std::size_t symbol_start = 0;
    const auto scale = [](cf32 x) { return x * kIfftScale; };
    for (std::size_t l = 0; l < kSymbolsPerSubframe; ++l) {
        // Move resource elements into their corresponding FFT bins
        const std::span<const cf32> sym = grid.symbol(l);
        std::ranges::transform(sym.subspan(0, kHalfSubcarriers),
                               ifft_.in().subspan(kFftSize - kHalfSubcarriers).begin(), scale);
        std::ranges::transform(sym.subspan(kHalfSubcarriers, kHalfSubcarriers),
                               ifft_.in().subspan(1).begin(), scale);
        ifft_.execute();

        // Move into time domain output and add cyclic prefix
        const std::size_t cp = cp_length(l);
        std::ranges::copy(ifft_.out(), out.subspan(symbol_start + cp).begin());
        std::ranges::copy(ifft_.out().subspan(kFftSize - cp, cp),
                          out.subspan(symbol_start).begin());
        symbol_start += kFftSize + cp;
    }
}
