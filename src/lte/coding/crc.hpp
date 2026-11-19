#pragma once

#include <cstdint>
#include <span>

namespace lte {

void crc24a(std::span<const std::uint8_t> bits, std::span<std::uint8_t> parity);
void crc24b(std::span<const std::uint8_t> bits, std::span<std::uint8_t> parity);
void crc16(std::span<const std::uint8_t> bits, std::span<std::uint8_t> parity);
void crc8(std::span<const std::uint8_t> bits, std::span<std::uint8_t> parity);

}  // namespace lte
