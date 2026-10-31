#include "lte/ofdm/ofdm_modulator.hpp"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <numbers>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include "golden/compare.hpp"
#include "golden/golden.hpp"
#include "lte/common/constants.hpp"
#include "lte/common/resource_grid.hpp"
#include "lte/common/types.hpp"

namespace {

// Start sample of OFDM symbol l (its cyclic prefix included) within the subframe.
std::size_t symbol_start(std::size_t l) {
    std::size_t start = 0;
    for (std::size_t i = 0; i < l; ++i) {
        const std::size_t cp =
            (i % lte::kSymbolsPerSlot == 0) ? lte::kCpLengthFirst : lte::kCpLengthOther;
        start += cp + lte::kFftSize;
    }
    return start;
}

}  // namespace

// The whole subframe against lteOFDMModulate (gen_ofdm.m: random QPSK grid, Windowing = 0).
TEST(OfdmModulator, MatchesLteOFDMModulate) {
    const auto grid_ref = golden::load<lte::cf32>("ofdm/resource_grid");
    const auto samples_ref = golden::load<lte::cf32>("ofdm/samples");

    // The C++ constants and the MATLAB configuration must describe the same carrier.
    ASSERT_EQ(grid_ref.config.at("NDLRB").get<std::size_t>(), lte::kNumRb);
    ASSERT_EQ(grid_ref.data.size(), lte::kNumSubcarriers * lte::kSymbolsPerSubframe);
    ASSERT_EQ(samples_ref.data.size(), lte::kSamplesPerSubframe);

    // Column-major 72 x 14: each run of 72 values is one OFDM symbol.
    lte::ResourceGrid grid{};
    const std::span<const lte::cf32> flat = grid_ref.data;
    for (std::size_t l = 0; l < lte::kSymbolsPerSubframe; ++l) {
        std::ranges::copy(flat.subspan(l * lte::kNumSubcarriers, lte::kNumSubcarriers),
                          grid.symbol(l).begin());
    }

    lte::OfdmModulator mod;
    std::vector<lte::cf32> out(lte::kSamplesPerSubframe);
    mod.modulate(grid, out);

    EXPECT_TRUE(golden::near(out, samples_ref.data, 1e-5));
}

// Diagnostic: one RE at grid index 36 (k = +1) in symbol 0, nothing else. Independent of
// scaling, symbol 0 must be a single tone rotating +2*pi/128 per sample across its cyclic
// prefix and body, and every other symbol must be silent. If MatchesLteOFDMModulate fails
// but this passes, look at scaling; if this fails, look at bin mapping or the cyclic prefix.
TEST(OfdmModulator, SingleSubcarrierIsOneTone) {
    lte::ResourceGrid grid{};
    grid.symbol(0)[lte::kHalfSubcarriers] = {1.0f, 0.0f};

    lte::OfdmModulator mod;
    std::vector<lte::cf32> out(lte::kSamplesPerSubframe);
    mod.modulate(grid, out);

    const std::size_t len = lte::kCpLengthFirst + lte::kFftSize;
    const std::span<const lte::cf32> sym0 = std::span<const lte::cf32>(out).first(len);
    ASSERT_GT(std::abs(sym0[0]), 0.0f) << "symbol 0 is empty";

    const lte::cf32 step = std::polar(1.0f, 2.0f * std::numbers::pi_v<float> / lte::kFftSize);
    for (std::size_t n = 1; n < len; ++n) {
        const lte::cf32 expected = sym0[n - 1] * step;
        ASSERT_LT(std::abs(sym0[n] - expected), 1e-4f * std::abs(sym0[0]))
            << "symbol 0 stops being a +1 tone at sample " << n;
    }

    for (std::size_t n = len; n < out.size(); ++n) {
        ASSERT_EQ(out[n], lte::cf32{}) << "energy outside symbol 0 at sample " << n;
    }
}

// Diagnostic: cyclic prefix lengths. Symbol l's prefix must repeat the last N_CP samples
// of its body: 10 for symbols 0 and 7, 9 for the rest.
TEST(OfdmModulator, CyclicPrefixRepeatsSymbolTail) {
    const auto grid_ref = golden::load<lte::cf32>("ofdm/resource_grid");
    lte::ResourceGrid grid{};
    const std::span<const lte::cf32> flat = grid_ref.data;
    for (std::size_t l = 0; l < lte::kSymbolsPerSubframe; ++l) {
        std::ranges::copy(flat.subspan(l * lte::kNumSubcarriers, lte::kNumSubcarriers),
                          grid.symbol(l).begin());
    }

    lte::OfdmModulator mod;
    std::vector<lte::cf32> out(lte::kSamplesPerSubframe);
    mod.modulate(grid, out);
    const std::span<const lte::cf32> all = out;

    for (std::size_t l = 0; l < lte::kSymbolsPerSubframe; ++l) {
        const std::size_t cp =
            (l % lte::kSymbolsPerSlot == 0) ? lte::kCpLengthFirst : lte::kCpLengthOther;
        const auto prefix = all.subspan(symbol_start(l), cp);
        const auto tail = all.subspan(symbol_start(l) + lte::kFftSize, cp);
        EXPECT_TRUE(std::ranges::equal(prefix, tail)) << "symbol " << l;
    }
}

TEST(OfdmModulatorDeathTest, WrongOutputSizeAborts) {
    lte::ResourceGrid grid{};
    lte::OfdmModulator mod;
    std::vector<lte::cf32> out(lte::kSamplesPerSubframe - 1);
    EXPECT_DEATH(mod.modulate(grid, out), "kSamplesPerSubframe");
}
