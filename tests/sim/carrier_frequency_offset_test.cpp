#include "lte/sim/carrier_frequency_offset.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstddef>
#include <limits>
#include <numbers>
#include <random>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include "lte/common/constants.hpp"
#include "lte/common/types.hpp"

namespace {

constexpr double kTwoPi = 2.0 * std::numbers::pi_v<double>;

double step_for(double cfo_hz) { return kTwoPi * cfo_hz / static_cast<double>(lte::kSampleRateHz); }

}  // namespace

// Zero CFO must leave every sample bit-for-bit unchanged, across several calls.
TEST(CarrierFrequencyOffset, ZeroCfoIsIdentity) {
    std::mt19937 rng(1);
    std::normal_distribution<float> n(0.0f, 1.0f);
    std::vector<lte::cf32> samples(3 * lte::kSamplesPerSubframe);
    for (auto &s : samples) {
        s = {n(rng), n(rng)};
    }
    const std::vector<lte::cf32> original = samples;

    lte::sim::CarrierFrequencyOffset cfo;
    const std::span<lte::cf32> all = samples;
    for (std::size_t i = 0; i < 3; ++i) {
        cfo.apply(all.subspan(i * lte::kSamplesPerSubframe, lte::kSamplesPerSubframe), 0.0);
    }
    for (std::size_t i = 0; i < samples.size(); ++i) {
        ASSERT_EQ(samples[i], original[i]) << "sample " << i;
    }
}

// A constant input through 10^6 samples, fed in uneven chunks, at a CFO that wraps the phase
// about 3,600 times. Every sample must have magnitude 1, every step between neighbours must be
// 2*pi*cfo/fs, and the absolute phase must still match n*step at the end. Catches a wrong
// multiplier, a wrap with the wrong period (a pi jump), a float accumulator drifting, and a
// phase that resets or skips between calls.
TEST(CarrierFrequencyOffset, PhaseStepsExactlyOverLongRun) {
    constexpr std::size_t kNumSamples = 1'000'000;
    constexpr std::array<std::size_t, 6> kChunks{1, 127, 1920, 999, 33'333, 4096};

    for (const double cfo_hz : {7000.0, -7000.0}) {
        SCOPED_TRACE(::testing::Message() << "cfo_hz = " << cfo_hz);
        const double step = step_for(cfo_hz);

        std::vector<lte::cf32> samples(kNumSamples, lte::cf32{1.0f, 0.0f});
        lte::sim::CarrierFrequencyOffset cfo;
        const std::span<lte::cf32> all = samples;
        std::size_t pos = 0;
        for (std::size_t c = 0; pos < kNumSamples; c = (c + 1) % kChunks.size()) {
            const std::size_t len = std::min(kChunks[c], kNumSamples - pos);
            cfo.apply(all.subspan(pos, len), cfo_hz);
            pos += len;
        }

        ASSERT_NEAR(std::abs(samples[0] - lte::cf32{1.0f, 0.0f}), 0.0f, 1e-7f)
            << "first sample must be unrotated";
        for (std::size_t i = 1; i < kNumSamples; ++i) {
            ASSERT_NEAR(std::abs(samples[i]), 1.0f, 1e-5f) << "magnitude at sample " << i;
            const double d = static_cast<double>(std::arg(samples[i] * std::conj(samples[i - 1])));
            ASSERT_NEAR(d, step, 1e-5) << "phase step between samples " << i - 1 << " and " << i;
        }

        // Absolute phase at the end, against n*step computed directly.
        const std::size_t last = kNumSamples - 1;
        const double expected = std::remainder(static_cast<double>(last) * step, kTwoPi);
        const double got = static_cast<double>(std::arg(samples[last]));
        EXPECT_NEAR(std::remainder(got - expected, kTwoPi), 0.0, 1e-4) << "phase drifted";
    }
}

// Sign convention: +15 kHz (one subcarrier spacing) moves a tone at bin k exactly to bin k+1,
// and -15 kHz to bin k-1. Compared sample by sample against the shifted tone.
TEST(CarrierFrequencyOffset, OneSubcarrierSpacingShiftsToneOneBin) {
    constexpr int kBin = 5;
    for (const int shift : {+1, -1}) {
        SCOPED_TRACE(::testing::Message() << "shift " << shift);
        std::vector<lte::cf32> samples(lte::kFftSize);
        const auto n_fft = static_cast<double>(lte::kFftSize);
        for (std::size_t n = 0; n < samples.size(); ++n) {
            const double angle = kTwoPi * kBin * static_cast<double>(n) / n_fft;
            samples[n] = {static_cast<float>(std::cos(angle)), static_cast<float>(std::sin(angle))};
        }

        lte::sim::CarrierFrequencyOffset cfo;
        cfo.apply(samples, shift * static_cast<double>(lte::kSubcarrierSpacingHz));

        for (std::size_t n = 0; n < samples.size(); ++n) {
            const double angle = kTwoPi * (kBin + shift) * static_cast<double>(n) / n_fft;
            const lte::cf32 want{static_cast<float>(std::cos(angle)),
                                 static_cast<float>(std::sin(angle))};
            ASSERT_LT(std::abs(samples[n] - want), 1e-5f) << "sample " << n;
        }
    }
}

TEST(CarrierFrequencyOffsetDeathTest, NonFiniteCfoAborts) {
    std::array<lte::cf32, 4> samples{};
    lte::sim::CarrierFrequencyOffset cfo;
    EXPECT_DEATH(cfo.apply(samples, std::numeric_limits<double>::quiet_NaN()), "isfinite");
    EXPECT_DEATH(cfo.apply(samples, std::numeric_limits<double>::infinity()), "isfinite");
}
