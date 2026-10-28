#pragma once

#include <cstdint>
#include <span>

#include "lte/common/types.hpp"

namespace lte {

void qpsk_map(std::span<const std::uint8_t> bits, std::span<lte::cf32> out);

}
