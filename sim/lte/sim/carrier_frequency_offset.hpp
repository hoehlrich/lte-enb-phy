#pragma once

#include <span>
#include "lte/common/types.hpp"

namespace lte::sim {

class CarrierFrequencyOffset {
public:
    void apply(std::span<cf32> samples, double cfo);

private:
    double phase_ = 0.0;

};

} // namespace lte::sim
