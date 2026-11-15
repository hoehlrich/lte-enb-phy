#include "lte/sim/carrier_frequency_offset.hpp"

#include <cmath>
#include <complex>
#include <numbers>
#include <span>

#include "lte/common/constants.hpp"
#include "lte/common/contract.hpp"
#include "lte/common/types.hpp"

void lte::sim::CarrierFrequencyOffset::apply(std::span<cf32> samples, double cfo_hz) {
    LTE_EXPECTS(std::isfinite(cfo_hz));

    const double step = (2 * std::numbers::pi_v<double> * cfo_hz) / kSampleRateHz;
    for (cf32 &sample : samples) {
        sample *= std::polar(1.0f, static_cast<float>(phase_));
        phase_ = std::fmod(phase_ + step, 2 * std::numbers::pi_v<double>);
    }
}
