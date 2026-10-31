#include "lte/ofdm/ofdm_demodulator.hpp"

#include <array>
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
#include "lte/ofdm/ofdm_modulator.hpp"

namespace {

std::size_t cp_length(std::size_t l) {
    return (l % lte::kSymbolsPerSlot == 0) ? lte::kCpLengthFirst : lte::kCpLengthOther;
}

// Start sample of OFDM symbol l (its cyclic prefix included) within the subframe.
std::size_t symbol_start(std::size_t l) {
    std::size_t start = 0;
    for (std::size_t i = 0; i < l; ++i) {
        start += cp_length(i) + lte::kFftSize;
    }
    return start;
}

// Column-major 72 x 14 (MATLAB order): each run of 72 values is one OFDM symbol.
std::vector<lte::cf32> flat_from_grid(const lte::ResourceGrid &grid) {
    std::vector<lte::cf32> flat;
    flat.reserve(lte::kNumSubcarriers * lte::kSymbolsPerSubframe);
    for (std::size_t l = 0; l < lte::kSymbolsPerSubframe; ++l) {
        for (const lte::cf32 re : grid.symbol(l)) {
            flat.push_back(re);
        }
    }
    return flat;
}

double rms(std::span<const lte::cf32> x) {
    double sum = 0.0;
    for (const lte::cf32 v : x) {
        sum += static_cast<double>(std::norm(v));
    }
    return std::sqrt(sum / static_cast<double>(x.size()));
}

// Grid index -> FFT bin (36.211 §6.12, 72 subcarriers in a 128-point FFT, DC unused).
// The two edges of each half, so an off-by-one in the mapping cannot hide.
struct ToneCase {
    std::size_t grid_index;
    std::size_t bin;
};
constexpr std::array<ToneCase, 4> kToneCases{{
    {0, 92},   // lowest subcarrier, k = -36
    {35, 127}, // k = -1, just below DC
    {36, 1},   // k = +1, just above DC
    {71, 36},  // highest subcarrier, k = +36
}};

}  // namespace

// The whole subframe against lteOFDMDemodulate, run on the lteOFDMModulate output from
// gen_ofdm.m. No scale constant on the C++ side: passing here is the scale measurement.
TEST(OfdmDemodulator, MatchesLteOFDMDemodulate) {
    const auto samples_ref = golden::load<lte::cf32>("ofdm/samples");
    const auto grid_ref = golden::load<lte::cf32>("ofdm/demodulated_grid");

    ASSERT_EQ(grid_ref.config.at("NDLRB").get<std::size_t>(), lte::kNumRb);
    ASSERT_EQ(samples_ref.data.size(), lte::kSamplesPerSubframe);
    ASSERT_EQ(grid_ref.data.size(), lte::kNumSubcarriers * lte::kSymbolsPerSubframe);

    lte::OfdmDemodulator demod;
    lte::ResourceGrid grid{};
    demod.demodulate(samples_ref.data, grid);
    const std::vector<lte::cf32> out = flat_from_grid(grid);

    // On failure, a clean constant ratio means a scale mismatch; a ratio of 1 means the bin
    // mapping or the FFT window is wrong (SingleToneLandsInOneRe will say which).
    EXPECT_TRUE(golden::near(out, grid_ref.data, 1e-5))
        << "RMS ratio C++ / Toolbox = " << rms(out) / rms(grid_ref.data);
}

// Modulate then demodulate gives back the grid with no scale factor. Every RE holds a
// distinct value encoding its position, so a swapped or shifted RE is caught and named.
TEST(OfdmDemodulator, RoundTripIsIdentity) {
    lte::ResourceGrid grid_in{};
    for (std::size_t l = 0; l < lte::kSymbolsPerSubframe; ++l) {
        const auto sym = grid_in.symbol(l);
        for (std::size_t k = 0; k < lte::kNumSubcarriers; ++k) {
            sym[k] = {static_cast<float>(k) - 35.5f, static_cast<float>(l) - 6.5f};
        }
    }

    lte::OfdmModulator mod;
    std::vector<lte::cf32> samples(lte::kSamplesPerSubframe);
    mod.modulate(grid_in, samples);

    lte::OfdmDemodulator demod;
    lte::ResourceGrid grid_out{};
    demod.demodulate(samples, grid_out);

    EXPECT_TRUE(golden::near(flat_from_grid(grid_out), flat_from_grid(grid_in), 1e-5));
    for (std::size_t l = 0; l < lte::kSymbolsPerSubframe; ++l) {
        for (std::size_t k = 0; k < lte::kNumSubcarriers; ++k) {
            ASSERT_LT(std::abs(grid_out.symbol(l)[k] - grid_in.symbol(l)[k]), 1e-3f)
                << "first wrong RE: symbol " << l << ", subcarrier " << k;
        }
    }
}

// Diagnostic, independent of the modulator and of scaling. A complex tone at one FFT bin,
// with its cyclic prefix, is placed in one symbol of an otherwise silent subframe. It must
// come out in exactly one RE with zero phase.
//   - Energy in the wrong RE: grid extraction (bin mapping).
//   - Right RE, nonzero phase: the FFT window starts at the wrong sample. An offset of d
//     samples rotates bin b by 2*pi*b*d/128.
//   - Energy in another symbol: the symbol boundaries (cyclic prefix lengths).
TEST(OfdmDemodulator, SingleToneLandsInOneRe) {
    constexpr std::array<std::size_t, 4> kSymbols{0, 1, 7, 13};  // both CP lengths, both slots

    lte::OfdmDemodulator demod;
    for (const std::size_t l : kSymbols) {
        for (const ToneCase &tc : kToneCases) {
            SCOPED_TRACE(::testing::Message() << "symbol " << l << ", grid index "
                                              << tc.grid_index << ", bin " << tc.bin);

            std::vector<lte::cf32> samples(lte::kSamplesPerSubframe);
            const std::size_t start = symbol_start(l);
            const std::size_t cp = cp_length(l);
            for (std::size_t n = 0; n < cp + lte::kFftSize; ++n) {
                // Phase index b*(n - cp) mod N, kept in integers so the angle stays exact.
                const std::size_t m = (tc.bin * (n + lte::kFftSize - cp)) % lte::kFftSize;
                const float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(m) /
                                    static_cast<float>(lte::kFftSize);
                samples[start + n] = std::polar(1.0f, angle);
            }

            lte::ResourceGrid grid{};
            demod.demodulate(samples, grid);

            const lte::cf32 target = grid.symbol(l)[tc.grid_index];
            ASSERT_GT(std::abs(target), 0.0f) << "tone missing from its RE";
            EXPECT_LT(std::abs(std::arg(target)), 1e-3f) << "tone has phase " << std::arg(target);

            const float limit = 1e-4f * std::abs(target);
            for (std::size_t ll = 0; ll < lte::kSymbolsPerSubframe; ++ll) {
                for (std::size_t k = 0; k < lte::kNumSubcarriers; ++k) {
                    if (ll == l && k == tc.grid_index) {
                        continue;
                    }
                    ASSERT_LT(std::abs(grid.symbol(ll)[k]), limit)
                        << "energy at symbol " << ll << ", subcarrier " << k;
                }
            }
        }
    }
}

TEST(OfdmDemodulatorDeathTest, WrongInputSizeAborts) {
    lte::OfdmDemodulator demod;
    lte::ResourceGrid grid{};
    std::vector<lte::cf32> samples(lte::kSamplesPerSubframe - 1);
    EXPECT_DEATH(demod.demodulate(samples, grid), "kSamplesPerSubframe");
}
