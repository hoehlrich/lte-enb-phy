#include "lte/sim/integer_delay.hpp"

#include <cstddef>
#include <span>
#include <utility>

#include "lte/common/types.hpp"

lte::sim::IntegerDelay::IntegerDelay(std::size_t d) : d_(d) {
    carryover_.resize(d);
}

void lte::sim::IntegerDelay::apply(std::span<cf32> samples) {
    if (d_ == 0) return;

    for (cf32 &s : samples) {
        std::swap(s, carryover_[pos_]);
        if (++pos_ == d_) pos_ = 0;
    }
}
