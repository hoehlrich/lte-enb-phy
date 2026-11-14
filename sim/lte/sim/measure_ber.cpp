#include "lte/sim/measure_ber.hpp"

#include <cstdint>

#include "lte/common/contract.hpp"
#include "lte/sim/thin_slice_link.hpp"

lte::sim::ErrorCount lte::sim::measure_ber(ThinSliceLink &link, float snr_db,
                                           std::uint64_t min_errors, std::uint64_t max_bits) {
    LTE_EXPECTS(min_errors > 0 && max_bits > 0);

    ErrorCount total;
    while (total.errors < min_errors && total.bits < max_bits) {
        total += link.run_subframe(snr_db);
    }
    return total;
}
