#pragma once

#include <cmath>

#include "lte/common/contract.hpp"

namespace lte {

// Decibel conversions for POWER ratios (SNR, noise variance): dB = 10 log10(ratio).
// Amplitude ratios use 20 log10 and are deliberately not provided here.

inline float db_to_power_ratio(float db) { return std::pow(10.0f, db / 10.0f); }

// Precondition: ratio > 0 (zero or negative power has no dB value).
inline float power_ratio_to_db(float ratio) {
    LTE_EXPECTS(ratio > 0.0f);
    return 10.0f * std::log10(ratio);
}

}  // namespace lte
