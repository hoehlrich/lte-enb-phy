#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <random>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include "golden/compare.hpp"
#include "golden/golden.hpp"
#include "lte/common/types.hpp"
#include "lte/mod/qpsk.hpp"

namespace {

// 36.211 Table 7.1.2-1, scaled by sqrt(2): b(2i) picks the sign of I, b(2i+1) the sign of Q.
struct Quadrant {
    float re;
    float im;
    std::uint8_t b0;
    std::uint8_t b1;
};
constexpr std::array<Quadrant, 4> kQuadrants{{
    {+1.0f, +1.0f, 0, 0},
    {+1.0f, -1.0f, 0, 1},
    {-1.0f, +1.0f, 1, 0},
    {-1.0f, -1.0f, 1, 1},
}};

}  // namespace

// Noisy symbols against lteSymbolDemodulate(..., 'QPSK', 'Hard') (gen_qpsk.m). The noise
// puts many symbols near the decision boundaries, so a wrong sign convention or bit order fails.
TEST(QpskDemap, MatchesLteSymbolDemodulate) {
    const auto symbols_ref = golden::load<lte::cf32>("mod/demap_symbols");
    const auto bits_ref = golden::load<std::uint8_t>("mod/demap_bits");
    ASSERT_EQ(bits_ref.data.size(), 2 * symbols_ref.data.size());

    std::vector<std::uint8_t> bits(bits_ref.data.size());
    lte::qpsk_demap_hard(symbols_ref.data, bits);

    const auto [got, want] = std::ranges::mismatch(bits, bits_ref.data);
    if (got != bits.end()) {
        const auto i = static_cast<std::size_t>(got - bits.begin());
        ADD_FAILURE() << "first wrong bit " << i << " (symbol " << i / 2 << " = "
                      << symbols_ref.data[i / 2] << "): got " << int{*got} << ", Toolbox "
                      << int{*want};
    }
    EXPECT_TRUE(golden::exact(bits, bits_ref.data));
}

// The four constellation points straight from the spec table, independent of MATLAB and of
// the mapper: catches a swapped b0/b1 or an inverted sign rule by name.
TEST(QpskDemap, ConstellationTable) {
    const float a = 1.0f / std::sqrt(2.0f);
    for (const Quadrant &q : kQuadrants) {
        SCOPED_TRACE(::testing::Message() << "point (" << q.re << ", " << q.im << ")/sqrt(2)");
        const std::array<lte::cf32, 1> symbol{lte::cf32{q.re * a, q.im * a}};
        std::array<std::uint8_t, 2> bits{0xAA, 0xAA};  // not 0 or 1: an unwritten bit shows up
        lte::qpsk_demap_hard(symbol, bits);
        EXPECT_EQ(bits[0], q.b0);
        EXPECT_EQ(bits[1], q.b1);
    }
}

// Equalized symbols do not sit on the unit circle: the decision must depend only on the sign
// of each axis, at any magnitude and with I and Q very unequal.
TEST(QpskDemap, DecisionDependsOnlyOnSign) {
    constexpr std::array<std::array<float, 2>, 7> kMagnitudes{{
        {1e-6f, 1e-6f},
        {1e-3f, 1e-3f},
        {0.3f, 0.3f},
        {7.0f, 7.0f},
        {1e4f, 1e4f},
        {1e-3f, 50.0f},
        {50.0f, 1e-3f},
    }};
    for (const Quadrant &q : kQuadrants) {
        for (const auto &m : kMagnitudes) {
            SCOPED_TRACE(::testing::Message()
                         << "symbol (" << q.re * m[0] << ", " << q.im * m[1] << ")");
            const std::array<lte::cf32, 1> symbol{lte::cf32{q.re * m[0], q.im * m[1]}};
            std::array<std::uint8_t, 2> bits{0xAA, 0xAA};
            lte::qpsk_demap_hard(symbol, bits);
            EXPECT_EQ(bits[0], q.b0);
            EXPECT_EQ(bits[1], q.b1);
        }
    }
}

// Map then demap gives back the bits exactly. No MATLAB needed.
TEST(QpskDemap, RoundTripThroughMapper) {
    constexpr std::size_t kNumBits = 2 * 1008;  // one subframe's worth of REs
    std::mt19937 rng(1);
    std::uniform_int_distribution<int> coin(0, 1);
    std::vector<std::uint8_t> bits_in(kNumBits);
    for (auto &b : bits_in) {
        b = static_cast<std::uint8_t>(coin(rng));
    }

    std::vector<lte::cf32> symbols(kNumBits / 2);
    lte::qpsk_map(bits_in, symbols);
    std::vector<std::uint8_t> bits_out(kNumBits);
    lte::qpsk_demap_hard(symbols, bits_out);

    EXPECT_TRUE(golden::exact(bits_out, bits_in));
}

TEST(QpskDemap, EmptyInputIsANoOp) {
    lte::qpsk_demap_hard(std::span<const lte::cf32>{}, std::span<std::uint8_t>{});
}

TEST(QpskDemapDeathTest, TooFewBitsAborts) {
    const std::vector<lte::cf32> symbols(4);
    std::vector<std::uint8_t> bits(7);
    EXPECT_DEATH(lte::qpsk_demap_hard(symbols, bits), "bits\\.size\\(\\)");
}

TEST(QpskDemapDeathTest, TooManyBitsAborts) {
    const std::vector<lte::cf32> symbols(4);
    std::vector<std::uint8_t> bits(9);
    EXPECT_DEATH(lte::qpsk_demap_hard(symbols, bits), "bits\\.size\\(\\)");
}
