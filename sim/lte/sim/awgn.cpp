#include "lte/sim/awgn.hpp"

#include <cmath>
#include <cstdint>
#include <random>
#include <span>

#include "lte/common/constants.hpp"
#include "lte/common/db.hpp"
#include "lte/common/types.hpp"

lte::sim::AwgnChannel::AwgnChannel(std::uint64_t seed) : mt_(seed) {}

void lte::sim::AwgnChannel::apply(std::span<cf32> samples, float snr_db) {
    float std_dev = sqrtf(1.0f / (2 * lte::kFftSize * db_to_power_ratio(snr_db)));
    std::normal_distribution d{0.0f, std_dev};

    for (cf32 &x : samples) {
        x += cf32{d(mt_), d(mt_)};
    }
}
