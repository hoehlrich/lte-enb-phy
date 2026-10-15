// The comparison helpers, tested by feeding them data that should fail.
#include "golden/compare.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace {

bool mentions(const ::testing::AssertionResult& r, const std::string& text) {
    return std::string(r.message()).find(text) != std::string::npos;
}

// Unit-power QPSK-like points: rms(expected) is exactly 1.
const std::vector<lte::cf32> kRef{{1, 0}, {0, 1}, {-1, 0}, {0, -1}};

}  // namespace

TEST(Exact, PassesOnIdenticalBits) {
    const std::vector<std::uint8_t> bits{1, 0, 1, 1, 0, 0};
    EXPECT_TRUE(golden::exact(bits, bits));
}

TEST(Exact, ReportsWhereBitsDiffer) {
    const std::vector<std::uint8_t> expected{1, 0, 1, 1, 0, 0};
    const std::vector<std::uint8_t> actual{1, 0, 0, 1, 0, 1};

    const auto r = golden::exact(actual, expected);
    EXPECT_FALSE(r);
    EXPECT_TRUE(mentions(r, "2 of 6 elements differ; first at index 2")) << r.message();
}

TEST(Exact, FailsOnSizeMismatch) {
    const std::vector<std::uint8_t> expected{1, 0, 1};
    const std::vector<std::uint8_t> actual{1, 0};
    EXPECT_FALSE(golden::exact(actual, expected));
}

TEST(Near, PassesWithinTolerance) {
    auto actual = kRef;
    actual[2] += lte::cf32{5e-6f, 0};
    EXPECT_TRUE(golden::near(actual, kRef, 1e-5));
}

TEST(Near, FailsOutsideTolerance) {
    auto actual = kRef;
    actual[2] += lte::cf32{2e-5f, 0};

    const auto r = golden::near(actual, kRef, 1e-5);
    EXPECT_FALSE(r);
    EXPECT_TRUE(mentions(r, "worst at index 2")) << r.message();
}

TEST(Near, ToleratesRoundingNoiseOnZeroReference) {
    // A null subcarrier: exactly zero in MATLAB, rounding noise in ours.
    const std::vector<lte::cf32> expected{{0, 0}, {1, 0}, {0, 0}, {-1, 0}};
    auto actual = expected;
    actual[0] = {1e-7f, -1e-7f};
    EXPECT_TRUE(golden::near(actual, expected, 1e-5));
}

TEST(Near, NaNFails) {
    auto actual = kRef;
    actual[1] = {std::numeric_limits<float>::quiet_NaN(), 0};

    const auto r = golden::near(actual, kRef, 1e-5);
    EXPECT_FALSE(r);
    EXPECT_TRUE(mentions(r, "worst at index 1")) << r.message();
}

TEST(Near, FailsOnSizeMismatch) {
    const std::vector<lte::cf32> actual(kRef.begin(), kRef.end() - 1);
    EXPECT_FALSE(golden::near(actual, kRef, 1e-5));
}
