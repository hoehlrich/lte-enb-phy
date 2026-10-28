#include "lte/mod/qpsk.hpp"

#include <vector>

#include <gtest/gtest.h>

#include "golden/compare.hpp"
#include "golden/golden.hpp"

TEST(Qpsk, MatchesLteSymbolModulate) {
    const auto bits = golden::load<std::uint8_t>("mod/qpsk_bits");
    const auto ref = golden::load<lte::cf32>("mod/qpsk_symbols");

    std::vector<lte::cf32> out(bits.data.size() / 2);
    lte::qpsk_map(bits.data, out);

    EXPECT_TRUE(golden::near(out, ref.data, 1e-5));
}

TEST(QpskDeathTest, OddBitCountsAborts) {
    std::vector<std::uint8_t> bits(3);
    std::vector<lte::cf32> out(1);
    EXPECT_DEATH(lte::qpsk_map(bits, out), "bits.size");
}
