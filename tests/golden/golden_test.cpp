// Round trip: matlab/gen_roundtrip.m writes known values, this reads them back.
#include "golden/golden.hpp"

#include <cstdint>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

TEST(Golden, LoadsBits) {
    const auto v = golden::load<std::uint8_t>("roundtrip/bits");

    EXPECT_EQ(v.shape, (std::vector<std::size_t>{8, 1}));
    EXPECT_EQ(v.data, (std::vector<std::uint8_t>{1, 0, 1, 1, 0, 0, 1, 0}));
    EXPECT_EQ(v.config.at("note"), "fixed pattern");
}

TEST(Golden, LoadsComplexInColumnMajorOrder) {
    const auto v = golden::load<lte::cf32>("roundtrip/samples");

    // MATLAB wrote [1+2i, -3+4i; 0.5-0.25i, 8-1i]: the left column comes first.
    // Every value is exact in float32, so exact comparison is correct here.
    const std::vector<lte::cf32> expected{{1, 2}, {0.5f, -0.25f}, {-3, 4}, {8, -1}};
    EXPECT_EQ(v.shape, (std::vector<std::size_t>{2, 2}));
    EXPECT_EQ(v.data, expected);
}

TEST(Golden, RejectsWrongType) {
    EXPECT_THROW(golden::load<float>("roundtrip/samples"), std::runtime_error);
}

TEST(Golden, RejectsMissingVector) {
    EXPECT_THROW(golden::load<float>("roundtrip/does_not_exist"), std::runtime_error);
}
