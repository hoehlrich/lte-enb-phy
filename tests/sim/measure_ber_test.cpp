#include "lte/sim/measure_ber.hpp"

#include <cmath>
#include <cstdint>

#include <gtest/gtest.h>

#include "lte/common/constants.hpp"
#include "lte/sim/thin_slice_link.hpp"

namespace {

constexpr std::uint64_t kBitsPerSubframe =
    2 * lte::kNumSubcarriers * lte::kSymbolsPerSubframe;  // 2016

// Uncoded QPSK in AWGN with SNR per RE = Es/N0: BER = 0.5 erfc(sqrt(Eb/N0)), Eb/N0 = Es/N0 / 2.
double qpsk_ber_theory(double snr_db) {
    const double ebn0 = std::pow(10.0, snr_db / 10.0) / 2.0;
    return 0.5 * std::erfc(std::sqrt(ebn0));
}

}  // namespace

// Every call sends exactly one subframe of payload bits.
TEST(ThinSliceLink, OneSubframeOfBits) {
    lte::sim::ThinSliceLink link(1);
    EXPECT_EQ(link.run_subframe(10.0f).bits, kBitsPerSubframe);
}

// With negligible noise the link must be error-free. Errors here mean a deterministic bug
// (bin mapping, cyclic prefix, bit slicing), which would show as a floor on the BER curve.
TEST(ThinSliceLink, NoErrorsWithoutNoise) {
    lte::sim::ThinSliceLink link(1);
    std::uint64_t errors = 0;
    for (int i = 0; i < 50; ++i) {
        errors += link.run_subframe(60.0f).errors;
    }
    EXPECT_EQ(errors, 0u);
}

// The thin-slice checkpoint, automated at one point. At 6 dB SNR per RE (Eb/N0 = 2.99 dB)
// theory gives BER = 0.0229. 2000 errors put the estimate within about 2.2% (1 sigma), so
// +/-10% is a ~4.5 sigma bound: it catches a 2.5 dB or 3 dB SNR-definition slip (BER off by
// a factor of 3 to 10) and will not flake.
TEST(MeasureBer, MatchesQpskTheoryAt6dB) {
    constexpr float kSnrDb = 6.0f;
    lte::sim::ThinSliceLink link(1);
    const lte::sim::ErrorCount count = lte::sim::measure_ber(link, kSnrDb, 2000, 10'000'000);

    ASSERT_GE(count.errors, 2000u) << "hit max_bits before min_errors";
    const double ber = static_cast<double>(count.errors) / static_cast<double>(count.bits);
    const double theory = qpsk_ber_theory(static_cast<double>(kSnrDb));
    EXPECT_NEAR(ber / theory, 1.0, 0.10) << "measured " << ber << ", theory " << theory;
}

// Stops on the first subframe that reaches min_errors. A twin link with the same seed, stepped
// one subframe at a time, says exactly where that is.
TEST(MeasureBer, StopsAtMinErrors) {
    constexpr std::uint64_t kMinErrors = 500;
    lte::sim::ThinSliceLink link(1);
    const lte::sim::ErrorCount count = lte::sim::measure_ber(link, 0.0f, kMinErrors, 10'000'000);

    lte::sim::ThinSliceLink twin(1);
    lte::sim::ErrorCount expected;
    while (expected.errors < kMinErrors) {
        expected += twin.run_subframe(0.0f);
    }
    EXPECT_EQ(count.bits, expected.bits);
    EXPECT_EQ(count.errors, expected.errors);
}

// At high SNR no errors arrive, so the bit cap ends the point after whole subframes.
TEST(MeasureBer, StopsAtMaxBits) {
    lte::sim::ThinSliceLink link(1);
    const std::uint64_t max_bits = 10 * kBitsPerSubframe + 1;
    const lte::sim::ErrorCount count = lte::sim::measure_ber(link, 60.0f, 100, max_bits);
    EXPECT_EQ(count.errors, 0u);
    EXPECT_EQ(count.bits, 11 * kBitsPerSubframe);
}

TEST(MeasureBer, SameSeedSameResult) {
    lte::sim::ThinSliceLink a(7);
    lte::sim::ThinSliceLink b(7);
    const auto ca = lte::sim::measure_ber(a, 3.0f, 300, 10'000'000);
    const auto cb = lte::sim::measure_ber(b, 3.0f, 300, 10'000'000);
    EXPECT_EQ(ca.bits, cb.bits);
    EXPECT_EQ(ca.errors, cb.errors);
}

TEST(MeasureBerDeathTest, ZeroLimitsAbort) {
    lte::sim::ThinSliceLink link(1);
    EXPECT_DEATH(lte::sim::measure_ber(link, 6.0f, 0, 1000), "min_errors");
    EXPECT_DEATH(lte::sim::measure_ber(link, 6.0f, 100, 0), "max_bits");
}
