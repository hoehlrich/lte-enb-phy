#include "lte/sim/integer_delay.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <random>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include "lte/common/constants.hpp"
#include "lte/common/types.hpp"

namespace {

std::vector<lte::cf32> random_samples(std::size_t n, unsigned seed) {
    std::mt19937 rng(seed);
    std::normal_distribution<float> g(0.0f, 1.0f);
    std::vector<lte::cf32> x(n);
    for (auto &s : x) {
        s = {g(rng), g(rng)};
    }
    return x;
}

// Feeds the whole buffer through the delay in chunks of the given sizes, cycling through them.
// Zero-length chunks are allowed and must change nothing.
void apply_in_chunks(lte::sim::IntegerDelay &delay, std::span<lte::cf32> all,
                     std::span<const std::size_t> chunks) {
    std::size_t pos = 0;
    for (std::size_t c = 0; pos < all.size(); c = (c + 1) % chunks.size()) {
        const std::size_t len = std::min(chunks[c], all.size() - pos);
        delay.apply(all.subspan(pos, len));
        pos += len;
    }
}

}  // namespace

// d = 0 must leave every sample bit-for-bit unchanged, across several calls.
TEST(IntegerDelay, ZeroDelayIsIdentity) {
    std::vector<lte::cf32> samples = random_samples(3 * lte::kSamplesPerSubframe, 1);
    const std::vector<lte::cf32> original = samples;

    lte::sim::IntegerDelay delay(0);
    constexpr std::array<std::size_t, 3> kChunks{lte::kSamplesPerSubframe, 1, 0};
    apply_in_chunks(delay, samples, kChunks);

    for (std::size_t i = 0; i < samples.size(); ++i) {
        ASSERT_EQ(samples[i], original[i]) << "sample " << i;
    }
}

// The signal has not arrived yet: the first d outputs of a fresh delay are zero.
TEST(IntegerDelay, FirstCallStartsWithDZeros) {
    constexpr std::size_t kD = 5;
    std::vector<lte::cf32> samples(20, lte::cf32{1.0f, 0.0f});
    lte::sim::IntegerDelay delay(kD);
    delay.apply(samples);

    for (std::size_t i = 0; i < samples.size(); ++i) {
        const lte::cf32 want = (i < kD) ? lte::cf32{} : lte::cf32{1.0f, 0.0f};
        ASSERT_EQ(samples[i], want) << "sample " << i;
    }
}

// A single impulse at index i must come out at exactly i + d and nowhere else. Covers the
// cyclic prefix lengths (9, 10), d = 1, and a d longer than one OFDM symbol.
TEST(IntegerDelay, ImpulseComesOutDLater) {
    constexpr std::size_t kImpulseAt = 100;
    for (const std::size_t d :
         {std::size_t{1}, std::size_t{9}, std::size_t{10}, std::size_t{200}}) {
        SCOPED_TRACE(::testing::Message() << "d = " << d);
        std::vector<lte::cf32> samples(lte::kSamplesPerSubframe);
        samples[kImpulseAt] = {1.0f, -2.0f};

        lte::sim::IntegerDelay delay(d);
        delay.apply(samples);

        for (std::size_t i = 0; i < samples.size(); ++i) {
            const lte::cf32 want = (i == kImpulseAt + d) ? lte::cf32{1.0f, -2.0f} : lte::cf32{};
            ASSERT_EQ(samples[i], want) << "sample " << i;
        }
    }
}

// The real contract: over any split into calls, output n equals input n - d (zero for n < d).
// Chunks include 0, 1, sizes below d, and a full subframe, so the carryover must survive calls
// shorter than the delay itself. d = 2000 is longer than a whole subframe.
TEST(IntegerDelay, StreamIsShiftedAcrossUnevenChunks) {
    constexpr std::size_t kNumSamples = 5 * lte::kSamplesPerSubframe;
    constexpr std::array<std::size_t, 7> kChunks{1, 0, 2, 3, 6, lte::kSamplesPerSubframe, 13};

    for (const std::size_t d :
         {std::size_t{1}, std::size_t{7}, std::size_t{9}, std::size_t{2000}}) {
        SCOPED_TRACE(::testing::Message() << "d = " << d);
        const std::vector<lte::cf32> input = random_samples(kNumSamples, 2);
        std::vector<lte::cf32> samples = input;

        lte::sim::IntegerDelay delay(d);
        apply_in_chunks(delay, samples, kChunks);

        for (std::size_t n = 0; n < kNumSamples; ++n) {
            const lte::cf32 want = (n < d) ? lte::cf32{} : input[n - d];
            ASSERT_EQ(samples[n], want) << "sample " << n;
        }
    }
}

// Two delays in series add: d1 then d2 equals one delay of d1 + d2.
TEST(IntegerDelay, DelaysCompose) {
    const std::vector<lte::cf32> input = random_samples(2 * lte::kSamplesPerSubframe, 3);
    std::vector<lte::cf32> two_stage = input;
    std::vector<lte::cf32> one_stage = input;

    lte::sim::IntegerDelay first(3);
    lte::sim::IntegerDelay second(4);
    lte::sim::IntegerDelay combined(7);
    first.apply(two_stage);
    second.apply(two_stage);
    combined.apply(one_stage);

    EXPECT_EQ(two_stage, one_stage);
}
