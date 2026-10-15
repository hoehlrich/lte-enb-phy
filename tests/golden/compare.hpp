#pragma once

#include <cstdint>
#include <span>

#include <gtest/gtest.h>

#include "lte/common/types.hpp"

namespace golden {

// Exact match, for bits and other integer data:
//   EXPECT_TRUE(golden::exact(bits, ref.data));
// On failure: how many elements differ and where the first ones are.
::testing::AssertionResult exact(std::span<const std::uint8_t> actual,
                                 std::span<const std::uint8_t> expected);
::testing::AssertionResult exact(std::span<const std::int32_t> actual,
                                 std::span<const std::int32_t> expected);

// Tolerance match, for samples:
//   EXPECT_TRUE(golden::near(waveform, ref.data, 1e-5));
// Passes when every |actual - expected| <= tol * rms(expected). Scaling by the
// reference's RMS makes tol independent of signal level, and keeps exact zeros
// in the reference (null subcarriers) from demanding exact zeros in ours.
// On failure: the worst element, how many exceed the limit, and the overall
// relative RMS error in dB.
::testing::AssertionResult near(std::span<const float> actual, std::span<const float> expected,
                                double tol);
::testing::AssertionResult near(std::span<const lte::cf32> actual,
                                std::span<const lte::cf32> expected, double tol);

}  // namespace golden
