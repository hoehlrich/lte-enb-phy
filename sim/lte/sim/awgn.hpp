#pragma once

#include <cstdint>
#include <random>
#include <span>

#include "lte/common/types.hpp"

namespace lte::sim {

class AwgnChannel {
public:
    AwgnChannel(std::uint64_t seed);

    // Adds complex white Gaussian noise in place. snr_db is per resource element
    // (Es/N0 after an unscaled reciever FFT), for a transmitter with unit-power symbols and 1/N
    // IFFT scaling: sigma^2 = 1 / (N * 10^(snr_db/10)).
    void apply(std::span<cf32> samples, float snr_db);

private:
    std::mt19937_64 mt_;
};

}  // namespace lte::sim
