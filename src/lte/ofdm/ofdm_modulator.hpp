#pragma once

#include <span>

#include "lte/common/fft.hpp"
#include "lte/common/resource_grid.hpp"
#include "lte/common/types.hpp"

namespace lte {

class OfdmModulator {
public:
    OfdmModulator();
    void modulate(const ResourceGrid &grid, std::span<cf32> out);

private:
    Fft ifft_;
};

}  // namespace lte
