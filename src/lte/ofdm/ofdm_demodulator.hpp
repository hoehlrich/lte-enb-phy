#pragma once

#include <span>

#include "lte/common/fft.hpp"
#include "lte/common/resource_grid.hpp"
#include "lte/common/types.hpp"

namespace lte {

class OfdmDemodulator {
public:
    OfdmDemodulator();
    void demodulate(std::span<const cf32> samples, ResourceGrid &grid);

private:
    Fft fft_;
};

}
