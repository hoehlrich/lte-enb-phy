#include "golden/compare.hpp"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <vector>

namespace golden {
namespace {

constexpr std::size_t kMaxListed = 8;

template <typename T>
::testing::AssertionResult exact_impl(std::span<const T> actual, std::span<const T> expected) {
    if (actual.size() != expected.size()) {
        return ::testing::AssertionFailure()
               << "size is " << actual.size() << ", expected " << expected.size();
    }

    std::vector<std::size_t> diffs;
    for (std::size_t i = 0; i < actual.size(); ++i) {
        if (actual[i] != expected[i]) {
            diffs.push_back(i);
        }
    }
    if (diffs.empty()) {
        return ::testing::AssertionSuccess();
    }

    // Unary + prints a uint8_t as a number instead of a character.
    const std::size_t first = diffs.front();
    auto result = ::testing::AssertionFailure();
    result << diffs.size() << " of " << actual.size() << " elements differ; first at index "
           << first << ": got " << +actual[first] << ", expected " << +expected[first]
           << "\n  first differing indices:";
    for (std::size_t k = 0; k < std::min(diffs.size(), kMaxListed); ++k) {
        result << ' ' << diffs[k];
    }
    if (diffs.size() > kMaxListed) {
        result << " ...";
    }
    return result;
}

// The checker does its own arithmetic in double, so its rounding never
// competes with the error it is measuring.
std::complex<double> widen(float x) { return {static_cast<double>(x), 0.0}; }
std::complex<double> widen(lte::cf32 x) {
    return {static_cast<double>(x.real()), static_cast<double>(x.imag())};
}

template <typename T>
::testing::AssertionResult near_impl(std::span<const T> actual, std::span<const T> expected,
                                     double tol) {
    if (actual.size() != expected.size()) {
        return ::testing::AssertionFailure()
               << "size is " << actual.size() << ", expected " << expected.size();
    }
    if (expected.empty()) {
        return ::testing::AssertionSuccess();
    }

    const auto n = static_cast<double>(expected.size());
    double ref_power = 0.0;
    for (const T& e : expected) {
        ref_power += std::norm(widen(e));
    }
    const double ref_rms = std::sqrt(ref_power / n);
    const double limit = tol * ref_rms;

    double err_power = 0.0;
    std::size_t over = 0;
    std::size_t worst = 0;
    double worst_err = 0.0;
    for (std::size_t i = 0; i < actual.size(); ++i) {
        const double err = std::abs(widen(actual[i]) - widen(expected[i]));
        err_power += err * err;
        // Written as !(err <= limit), not err > limit, so a NaN counts as a
        // failure: every comparison with NaN is false.
        if (!(err <= limit)) {
            ++over;
        }
        if (std::isnan(worst_err)) {
            continue;  // the first NaN stays the worst element
        }
        if (std::isnan(err) || err > worst_err) {
            worst_err = err;
            worst = i;
        }
    }
    if (over == 0) {
        return ::testing::AssertionSuccess();
    }

    // Formatted here because GoogleTest prints doubles to 17 digits.
    const double rel_rms = std::sqrt(err_power / n) / ref_rms;
    std::ostringstream msg;
    msg << over << " of " << actual.size() << " elements exceed " << tol << " x rms(expected)\n"
        << std::setprecision(7) << "  worst at index " << worst << ": got " << actual[worst]
        << ", expected " << expected[worst] << std::setprecision(3) << ", error "
        << worst_err / ref_rms << " x rms(expected)\n"
        << "  relative rms error " << rel_rms << " (" << std::fixed << std::setprecision(1)
        << 20.0 * std::log10(rel_rms) << " dB)";
    return ::testing::AssertionFailure() << msg.str();
}

}  // namespace

::testing::AssertionResult exact(std::span<const std::uint8_t> actual,
                                 std::span<const std::uint8_t> expected) {
    return exact_impl(actual, expected);
}
::testing::AssertionResult exact(std::span<const std::int32_t> actual,
                                 std::span<const std::int32_t> expected) {
    return exact_impl(actual, expected);
}
::testing::AssertionResult near(std::span<const float> actual, std::span<const float> expected,
                                double tol) {
    return near_impl(actual, expected, tol);
}
::testing::AssertionResult near(std::span<const lte::cf32> actual,
                                std::span<const lte::cf32> expected, double tol) {
    return near_impl(actual, expected, tol);
}

}  // namespace golden
