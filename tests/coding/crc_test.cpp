#include "lte/coding/crc.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <span>
#include <string>
#include <tuple>
#include <vector>

#include <gtest/gtest.h>

#include "golden/compare.hpp"
#include "golden/golden.hpp"

namespace {

using CrcFn = void (*)(std::span<const std::uint8_t>, std::span<std::uint8_t>);

// One entry per generator polynomial in 36.212 §5.1.1. `exponents` lists the nonzero terms of
// g(D), copied from the spec rather than from the hex constants in crc.cpp, so the long-division
// reference below shares nothing with the implementation.
struct Crc {
    const char *name;  // vector names are coding/crc<name>_n<length>_{bits,parity}
    const char *poly;  // as lteCRCEncode spells it; gen_crc.m stores it in the config
    std::size_t L;     // parity bits
    CrcFn fn;
    std::vector<std::size_t> exponents;
};

const std::array<Crc, 4> kCrcs{{
    {"24a", "24A", 24, &lte::crc24a, {24, 23, 18, 17, 14, 11, 10, 7, 6, 5, 4, 3, 1, 0}},
    {"24b", "24B", 24, &lte::crc24b, {24, 23, 6, 5, 1, 0}},
    {"16", "16", 16, &lte::crc16, {16, 12, 5, 0}},
    {"8", "8", 8, &lte::crc8, {8, 7, 4, 3, 1, 0}},
}};

// The vector lengths gen_crc.m writes. All whole bytes, as the contract requires.
constexpr std::array<std::size_t, 4> kVectorLengths{8, 40, 2048, 6120};

// 36.212 §5.1.1 taken literally: p(D) = a(D)·D^L mod g(D), by long division one bit at a time.
// No table, no packing, no shift register.
std::vector<std::uint8_t> reference_parity(const Crc &crc, std::span<const std::uint8_t> bits) {
    std::vector<std::uint8_t> g(crc.L + 1, 0);  // g[0] is the D^L coefficient
    for (std::size_t e : crc.exponents) {
        g[crc.L - e] = 1;
    }
    std::vector<std::uint8_t> work(bits.begin(), bits.end());
    work.resize(bits.size() + crc.L, 0);  // a(D)·D^L
    for (std::size_t k = 0; k < bits.size(); ++k) {
        if (work[k] != 0) {
            for (std::size_t t = 0; t <= crc.L; ++t) {
                work[k + t] ^= g[t];
            }
        }
    }
    return {work.end() - static_cast<std::ptrdiff_t>(crc.L), work.end()};
}

std::vector<std::uint8_t> random_bits(std::size_t n, std::uint32_t seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> coin(0, 1);
    std::vector<std::uint8_t> bits(n);
    for (auto &b : bits) {
        b = static_cast<std::uint8_t>(coin(rng));
    }
    return bits;
}

// Runs the CRC into a parity buffer prefilled with 0xAA: not 0 or 1, so a bit the function never
// writes shows up in any comparison.
std::vector<std::uint8_t> parity_of(const Crc &crc, std::span<const std::uint8_t> bits) {
    std::vector<std::uint8_t> parity(crc.L, 0xAA);
    crc.fn(bits, parity);
    return parity;
}

std::string crc_name(std::size_t index) { return "crc" + std::string{kCrcs[index].name}; }

}  // namespace

// ---------------------------------------------------------------------------------------------
// Golden vectors: every CRC at every length against lteCRCEncode (gen_crc.m).

class CrcGolden : public ::testing::TestWithParam<std::tuple<std::size_t, std::size_t>> {};

TEST_P(CrcGolden, MatchesLteCrcEncode) {
    const auto [index, length] = GetParam();
    const Crc &crc = kCrcs[index];
    const std::string base = "coding/crc" + std::string{crc.name} + "_n" + std::to_string(length);
    const auto bits = golden::load<std::uint8_t>(base + "_bits");
    const auto want = golden::load<std::uint8_t>(base + "_parity");

    // The generator's config must agree with the name we loaded it by; if not, the vectors
    // were regenerated wrong and the comparison below would mean nothing.
    ASSERT_EQ(want.config.at("poly").get<std::string>(), crc.poly);
    ASSERT_EQ(want.config.at("length").get<std::size_t>(), length);
    ASSERT_EQ(want.config.at("parity_bits").get<std::size_t>(), crc.L);
    ASSERT_EQ(bits.data.size(), length);
    ASSERT_EQ(want.data.size(), crc.L);

    EXPECT_TRUE(golden::exact(parity_of(crc, bits.data), want.data));
}

INSTANTIATE_TEST_SUITE_P(Crc, CrcGolden,
                         ::testing::Combine(::testing::Range(std::size_t{0}, kCrcs.size()),
                                            ::testing::ValuesIn(kVectorLengths)),
                         [](const auto &param_info) {
                             return crc_name(std::get<0>(param_info.param)) + "_n" +
                                    std::to_string(std::get<1>(param_info.param));
                         });

// ---------------------------------------------------------------------------------------------
// Properties from the spec's definition. No MATLAB needed.

class CrcProperty : public ::testing::TestWithParam<std::size_t> {};

// The table-driven version against long division, over many lengths. Catches packing and
// table-indexing bugs at lengths the golden vectors don't cover.
TEST_P(CrcProperty, MatchesLongDivision) {
    const Crc &crc = kCrcs[GetParam()];
    constexpr std::array<std::size_t, 9> kBytes{0, 1, 2, 3, 5, 8, 17, 64, 765};
    std::uint32_t seed = 1;
    for (std::size_t bytes : kBytes) {
        const auto bits = random_bits(8 * bytes, seed++);
        SCOPED_TRACE(::testing::Message() << bits.size() << " bits");
        EXPECT_TRUE(golden::exact(parity_of(crc, bits), reference_parity(crc, bits)));
    }
}

// A single 1 at each position of a 3-byte message. Each position gives a different parity, so a
// bit-order mistake in packing (a0 not in the MSB) or in unpacking (p0 not from bit 31) fails
// here, and the trace names the position.
TEST_P(CrcProperty, SingleOneAtEachPosition) {
    const Crc &crc = kCrcs[GetParam()];
    for (std::size_t pos = 0; pos < 24; ++pos) {
        std::vector<std::uint8_t> bits(24, 0);
        bits[pos] = 1;
        SCOPED_TRACE(::testing::Message() << "a" << pos << " = 1");
        EXPECT_TRUE(golden::exact(parity_of(crc, bits), reference_parity(crc, bits)));
    }
}

// The register starts at zero (no init value), so zero input leaves zero remainder.
TEST_P(CrcProperty, AllZerosGiveZeroParity) {
    const Crc &crc = kCrcs[GetParam()];
    const std::vector<std::uint8_t> zeros_parity(crc.L, 0);
    for (std::size_t n : {std::size_t{0}, std::size_t{8}, std::size_t{512}}) {
        SCOPED_TRACE(::testing::Message() << n << " zero bits");
        EXPECT_TRUE(golden::exact(parity_of(crc, std::vector<std::uint8_t>(n, 0)), zeros_parity));
    }
}

// Leading zeros don't change a(D)·D^L, so they can't change the parity. With a nonzero initial
// register they would. Segmentation relies on this: filler bits are zeros at the front of the
// first code block.
TEST_P(CrcProperty, LeadingZerosDoNotChangeParity) {
    const Crc &crc = kCrcs[GetParam()];
    const auto bits = random_bits(40, 7);
    const auto expected = parity_of(crc, bits);
    for (std::size_t zeros : {std::size_t{8}, std::size_t{64}}) {
        std::vector<std::uint8_t> padded(zeros, 0);
        padded.insert(padded.end(), bits.begin(), bits.end());
        SCOPED_TRACE(::testing::Message() << zeros << " leading zeros");
        EXPECT_TRUE(golden::exact(parity_of(crc, padded), expected));
    }
}

// The spec's own condition: the systematic codeword a0..aA-1 p0..pL-1 divides by g(D) with no
// remainder, so its CRC is zero. Every L is a multiple of 8, so the codeword is whole bytes.
TEST_P(CrcProperty, CodewordLeavesZeroRemainder) {
    const Crc &crc = kCrcs[GetParam()];
    for (std::uint32_t seed = 1; seed <= 4; ++seed) {
        auto codeword = random_bits(64 * seed, seed);
        const auto parity = parity_of(crc, codeword);
        codeword.insert(codeword.end(), parity.begin(), parity.end());
        SCOPED_TRACE(::testing::Message() << "seed " << seed);
        EXPECT_TRUE(golden::exact(parity_of(crc, codeword), std::vector<std::uint8_t>(crc.L, 0)));
    }
}

INSTANTIATE_TEST_SUITE_P(Crc, CrcProperty, ::testing::Range(std::size_t{0}, kCrcs.size()),
                         [](const auto &param_info) { return crc_name(param_info.param); });

// ---------------------------------------------------------------------------------------------
// Contracts: each one aborts with the failed condition.

class CrcDeathTest : public ::testing::TestWithParam<std::size_t> {};

TEST_P(CrcDeathTest, ParityTooShortAborts) {
    const Crc &crc = kCrcs[GetParam()];
    const std::vector<std::uint8_t> bits(8, 0);
    std::vector<std::uint8_t> parity(crc.L - 1);
    EXPECT_DEATH(crc.fn(bits, parity), "parity\\.size\\(\\)");
}

TEST_P(CrcDeathTest, ParityTooLongAborts) {
    const Crc &crc = kCrcs[GetParam()];
    const std::vector<std::uint8_t> bits(8, 0);
    std::vector<std::uint8_t> parity(crc.L + 1);
    EXPECT_DEATH(crc.fn(bits, parity), "parity\\.size\\(\\)");
}

TEST_P(CrcDeathTest, PartialByteAborts) {
    const Crc &crc = kCrcs[GetParam()];
    const std::vector<std::uint8_t> bits(12, 0);
    std::vector<std::uint8_t> parity(crc.L);
    EXPECT_DEATH(crc.fn(bits, parity), "bits\\.size\\(\\) % 8");
}

// Non-binary elements at the start of a byte and in the middle of one, since the first element
// of each byte is easy to leave unchecked.
TEST_P(CrcDeathTest, NonBinaryBitAborts) {
    const Crc &crc = kCrcs[GetParam()];
    for (std::size_t pos : {std::size_t{0}, std::size_t{8}, std::size_t{13}}) {
        SCOPED_TRACE(::testing::Message() << "bits[" << pos << "] = 2");
        std::vector<std::uint8_t> bits(16, 0);
        bits[pos] = 2;
        std::vector<std::uint8_t> parity(crc.L);
        EXPECT_DEATH(crc.fn(bits, parity), "contract violated");
    }
}

INSTANTIATE_TEST_SUITE_P(Crc, CrcDeathTest, ::testing::Range(std::size_t{0}, kCrcs.size()),
                         [](const auto &param_info) { return crc_name(param_info.param); });
