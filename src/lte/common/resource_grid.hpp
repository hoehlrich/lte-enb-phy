#pragma once

#include <array>
#include <cstddef>
#include <span>

#include "lte/common/constants.hpp"
#include "lte/common/contract.hpp"
#include "lte/common/types.hpp"

namespace lte {

// RE (k, l) is `symbol(l)[k]`. `l` runs 0–13 across the subframe, while 36.211’s `l` restarts at 0
// each slot, so the spec’s (k, l) in slot 1 is `symbol(l + 7)[k]`
struct ResourceGrid {
    std::span<cf32> symbol(std::size_t l) {
        LTE_EXPECTS(l < kSymbolsPerSubframe);
        return std::span{elements[l]};
    }

    std::span<const cf32> symbol(std::size_t l) const {
        LTE_EXPECTS(l < kSymbolsPerSubframe);
        return std::span{elements[l]};
    }

    std::array<std::array<cf32, kNumSubcarriers>, kSymbolsPerSubframe> elements;
};

}  // namespace lte
