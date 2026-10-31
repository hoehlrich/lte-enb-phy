#pragma once

#include <cstddef>

namespace lte {

// 36.211 Table §6.2.3-1 (normal CP), 1.4 MHz
inline constexpr std::size_t kNumRb = 6;              // N_RB
inline constexpr std::size_t kSubcarriersPerRb = 12;  // N_sc^RB
inline constexpr std::size_t kNumSubcarriers = kNumRb * kSubcarriersPerRb;
inline constexpr std::size_t kSymbolsPerSlot = 7;  // N_symb, normal CP
inline constexpr std::size_t kSymbolsPerSubframe = 2 * kSymbolsPerSlot;

// 36.211 §6.12 (normal CP, Δf = 15kHz), 1.4 MHz
// The spec gives lengths in T_s = 1 / (15000 * 2048) s, i.e. for a 2048-point FFT.
// This implementation uses a 128-point FFT, so every T_s length is divided by 16
inline constexpr std::size_t kSpecFftSize = 2048;  // N in §6.12
inline constexpr std::size_t kFftSize = 128;
inline constexpr std::size_t kTsPerSample = kSpecFftSize / kFftSize;  // 16
inline constexpr std::size_t kHalfSubcarriers = kNumSubcarriers / 2;  // N_RB^DL N_SC^RB / 2

inline constexpr std::size_t kCpLengthFirst = 160 / kTsPerSample;  // N_CP,0 = 160
inline constexpr std::size_t kCpLengthOther = 144 / kTsPerSample;  // N_CP,l = 144
inline constexpr std::size_t cp_length(std::size_t l) {
    return (l % kSymbolsPerSlot == 0) ? kCpLengthFirst : kCpLengthOther;
}

inline constexpr std::size_t kSamplesPerSlot =
    (kCpLengthFirst + kFftSize) + (kSymbolsPerSlot - 1) * (kCpLengthOther + kFftSize);
inline constexpr std::size_t kSamplesPerSubframe = 2 * kSamplesPerSlot;

static_assert(160 % kTsPerSample == 0 && 144 % kTsPerSample == 0,
              "CP lengths must be whole samples at this FFT size");
static_assert(kSamplesPerSlot * kTsPerSample == 15360, "T_slot = 15360 T_s (36.211 §4)");

}  // namespace lte
