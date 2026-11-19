#include "lte/coding/crc.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

#include "lte/common/contract.hpp"

namespace {

constexpr std::size_t kTableSize = std::numeric_limits<std::uint8_t>::max() + 1;

constexpr std::array<std::uint32_t, kTableSize> make_table(std::uint32_t poly) {
    std::array<std::uint32_t, kTableSize> table{};
    for (std::uint32_t byte = 0; byte < kTableSize; ++byte) {
        std::uint32_t r = byte << 24;
        for (std::size_t shift = 0; shift < 8; ++shift) {
            bool do_xor = (r & 0x8000'0000u) != 0;
            r <<= 1;
            if (do_xor) r ^= poly;
        }
        table[byte] = r;
    }
    return table;
}

constexpr std::uint32_t kCrc24aPoly = 0x864CFBu << 8;
constexpr auto kCrc24aTable = make_table(kCrc24aPoly);
static_assert(kCrc24aTable[0] == 0);
static_assert(kCrc24aTable[1] == kCrc24aPoly);
static_assert(kCrc24aTable[3] == (kCrc24aTable[1] ^ kCrc24aTable[2]));

constexpr std::uint32_t kCrc24bPoly = 0x800063u << 8;
constexpr auto kCrc24bTable = make_table(kCrc24bPoly);
static_assert(kCrc24bTable[0] == 0);
static_assert(kCrc24bTable[1] == kCrc24bPoly);
static_assert(kCrc24bTable[3] == (kCrc24bTable[1] ^ kCrc24bTable[2]));

constexpr std::uint32_t kCrc16Poly = 0x1021u << 16;
constexpr auto kCrc16Table = make_table(kCrc16Poly);
static_assert(kCrc16Table[0] == 0);
static_assert(kCrc16Table[1] == kCrc16Poly);
static_assert(kCrc16Table[3] == (kCrc16Table[1] ^ kCrc16Table[2]));

constexpr std::uint32_t kCrc8Poly = 0x9Bu << 24;
constexpr auto kCrc8Table = make_table(kCrc8Poly);
static_assert(kCrc8Table[0] == 0);
static_assert(kCrc8Table[1] == kCrc8Poly);
static_assert(kCrc8Table[3] == (kCrc8Table[1] ^ kCrc8Table[2]));

}  // namespace

void lte::crc8(std::span<const std::uint8_t> bits, std::span<std::uint8_t> parity) {
    LTE_EXPECTS(parity.size() == 8);
    LTE_EXPECTS(bits.size() % 8 == 0);

    std::uint32_t r = 0;
    for (std::size_t i = 0; i < bits.size() / 8; ++i) {
        std::uint8_t message_byte = bits[i * 8];
        LTE_EXPECTS(message_byte == 0 || message_byte == 1);
        for (std::size_t j = 1; j < 8; ++j) {
            message_byte = static_cast<std::unit8_t>(message_byte << 1);
            std::uint8_t bit = bits[i * 8 + j];
            LTE_EXPECTS(bit == 0 || bit == 1);
            message_byte |= bit;
        }
        r = (r << 8) ^ kCrc8Table[(r >> 24) ^ message_byte];
    }

    for (std::size_t i = 0; i < 8; ++i)
        parity[i] = static_cast<std::uint8_t>((r << i) >> 31);
}

