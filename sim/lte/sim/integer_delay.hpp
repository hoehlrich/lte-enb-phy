#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "lte/common/types.hpp"

namespace lte::sim {

class IntegerDelay {
public:
    IntegerDelay(std::size_t d);

    void apply(std::span<cf32> samples);

private:
    const std::size_t d_;
    std::vector<cf32> carryover_;
    std::size_t pos_ = 0;
};

}  // namespace lte::sim
