#include "lte/sim/awgn.hpp"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include "lte/common/constants.hpp"
#include "lte/common/db.hpp"
#include "lte/common/fft.hpp"
#include "lte/common/types.hpp"

// The channel adds noise with per-sample variance sigma^2 = 1 / (N * SNR_lin), so that
// after an unscaled N-point receiver FFT each resource element sees noise variance
// 1 / SNR_lin against unit-power symbols (the project's per-RE SNR definition).
//
// The checks are statistical. With ~1e6 samples the estimates are within ~0.3% (3 sigma)
// of their true values, so the 2% tolerances below fail only on a real bug, not by chance.

namespace {

constexpr std::size_t kNumSamples = 1'000'000;
constexpr std::uint64_t kSeed = 1;

double mean_power(std::span<const lte::cf32> x) {
    double sum = 0.0;
    for (const lte::cf32 v : x) {
        sum += static_cast<double>(std::norm(v));
    }
    return sum / static_cast<double>(x.size());
}

// Noise added to an all-zero signal is just the noise.
std::vector<lte::cf32> noise_only(float snr_db, std::uint64_t seed = kSeed) {
    std::vector<lte::cf32> x(kNumSamples);
    lte::sim::AwgnChannel channel(seed);
    channel.apply(x, snr_db);
    return x;
}

double expected_sample_variance(float snr_db) {
    return 1.0 / (static_cast<double>(lte::kFftSize) *
                  static_cast<double>(lte::db_to_power_ratio(snr_db)));
}

}  // namespace

// Per time-domain sample: E|w|^2 = 1 / (N * SNR_lin), across a range of SNRs.
TEST(Awgn, SampleVarianceMatchesSnr) {
    for (const float snr_db : {-5.0f, 0.0f, 10.0f, 30.0f}) {
        const auto w = noise_only(snr_db);
        const double expected = expected_sample_variance(snr_db);
        EXPECT_NEAR(mean_power(w) / expected, 1.0, 0.02) << snr_db << " dB";
    }
}

// The noise is circular: zero mean, and real and imaginary parts each carry half the power.
TEST(Awgn, NoiseIsCircular) {
    const auto w = noise_only(10.0f);
    const double expected = expected_sample_variance(10.0f);

    double sum_re = 0.0, sum_im = 0.0, pow_re = 0.0, pow_im = 0.0;
    for (const lte::cf32 v : w) {
        sum_re += static_cast<double>(v.real());
        sum_im += static_cast<double>(v.imag());
        pow_re += static_cast<double>(v.real() * v.real());
        pow_im += static_cast<double>(v.imag() * v.imag());
    }
    const auto n = static_cast<double>(w.size());
    const double sigma = std::sqrt(expected / 2.0);

    // Sample mean of n Gaussians has standard deviation sigma / sqrt(n); allow 5 of them.
    EXPECT_NEAR(sum_re / n, 0.0, 5.0 * sigma / std::sqrt(n));
    EXPECT_NEAR(sum_im / n, 0.0, 5.0 * sigma / std::sqrt(n));
    EXPECT_NEAR(pow_re / n / (expected / 2.0), 1.0, 0.02);
    EXPECT_NEAR(pow_im / n / (expected / 2.0), 1.0, 0.02);
}

// The definition itself: after an unscaled N-point FFT, noise per bin is 1 / SNR_lin.
TEST(Awgn, PerResourceElementNoiseIsOneOverSnr) {
    const float snr_db = 10.0f;
    const auto w = noise_only(snr_db);

    lte::Fft fft(lte::kFftSize, lte::Fft::Direction::kForward);
    double sum = 0.0;
    std::size_t bins = 0;
    for (std::size_t start = 0; start + lte::kFftSize <= w.size(); start += lte::kFftSize) {
        std::ranges::copy(std::span(w).subspan(start, lte::kFftSize), fft.in().begin());
        fft.execute();
        for (const lte::cf32 v : fft.out()) {
            sum += static_cast<double>(std::norm(v));
        }
        bins += lte::kFftSize;
    }
    const double per_re = sum / static_cast<double>(bins);
    EXPECT_NEAR(per_re * static_cast<double>(lte::db_to_power_ratio(snr_db)), 1.0, 0.02);
}

// The channel adds to the signal in place: the output is the input plus exactly the noise
// that the same seed adds to zeros.
TEST(Awgn, AddsToSignal) {
    std::vector<lte::cf32> x(lte::kSamplesPerSubframe, lte::cf32{0.5f, -0.25f});
    std::vector<lte::cf32> zeros(x.size());

    lte::sim::AwgnChannel a(kSeed), b(kSeed);
    a.apply(x, 10.0f);
    b.apply(zeros, 10.0f);

    for (std::size_t i = 0; i < x.size(); ++i) {
        ASSERT_EQ(x[i], lte::cf32(0.5f, -0.25f) + zeros[i]) << "sample " << i;
    }
}

// Same seed, same noise; different seed, different noise; and the stream continues across
// calls instead of restarting.
TEST(Awgn, SeedControlsTheStream) {
    std::vector<lte::cf32> a(64), b(64), c(64);
    lte::sim::AwgnChannel ca(kSeed), cb(kSeed), cc(kSeed + 1);
    ca.apply(a, 0.0f);
    cb.apply(b, 0.0f);
    cc.apply(c, 0.0f);
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);

    std::vector<lte::cf32> next(64);
    ca.apply(next, 0.0f);
    EXPECT_NE(a, next) << "second call repeated the first call's noise";
}
