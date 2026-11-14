#pragma once

#include <cstdint>

#include "lte/sim/thin_slice_link.hpp"

namespace lte::sim {

inline ErrorCount &operator+=(ErrorCount &total, const ErrorCount &more) {
    total.bits += more.bits;
    total.errors += more.errors;
    return total;
}

// Runs whole subframes at snr_db (per RE) until at least min_errors bit errors are counted or
// at least max_bits bits have been sent, whichever comes first. Totals may overshoot either
// limit by up to one subframe. With ~100 errors the BER estimate is good to about +/-10%.
// Precondition: min_errors > 0 and max_bits > 0.
ErrorCount measure_ber(ThinSliceLink &link, float snr_db, std::uint64_t min_errors,
                       std::uint64_t max_bits);

}  // namespace lte::sim
