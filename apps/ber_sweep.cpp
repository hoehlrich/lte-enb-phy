// ber_sweep: BER vs. SNR for the thin slice (uncoded QPSK, OFDM, AWGN).
// Writes one CSV row per SNR point for matlab/plot_ber.m. Progress goes to stderr.
//
//   ber_sweep [--snr-start dB] [--snr-stop dB] [--snr-step dB] [--min-errors N]
//             [--max-bits N] [--seed N] [--out path]
//
// SNR is per resource element (Es/N0 after an unscaled receiver FFT); Eb/N0 = SNR - 10 log10(2).

#include <chrono>
#include <cinttypes>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>

#include "lte/sim/measure_ber.hpp"
#include "lte/sim/thin_slice_link.hpp"

namespace {

struct Options {
    double snr_start_db = 0.0;
    double snr_stop_db = 12.0;
    double snr_step_db = 1.0;
    std::uint64_t min_errors = 100;
    std::uint64_t max_bits = 100'000'000;
    std::uint64_t seed = 1;
    std::string out = "ber_thin_slice.csv";
};

constexpr std::string_view kUsage =
    "usage: ber_sweep [--snr-start dB] [--snr-stop dB] [--snr-step dB] [--min-errors N]\n"
    "                 [--max-bits N] [--seed N] [--out path]\n"
    "defaults: 0 12 1 100 1e8 1 ber_thin_slice.csv; SNR is per RE (Es/N0)\n";

std::optional<double> parse_double(const char *s) {
    char *end = nullptr;
    const double v = std::strtod(s, &end);
    if (end == s || *end != '\0' || !std::isfinite(v)) {
        return std::nullopt;
    }
    return v;
}

// Accepts integers and scientific notation ("1e8"); rejects fractions and values that a
// double cannot represent exactly.
std::optional<std::uint64_t> parse_count(const char *s) {
    const auto v = parse_double(s);
    if (!v || *v < 0.0 || *v > 9.007199254740992e15 || *v != std::floor(*v)) {
        return std::nullopt;
    }
    return static_cast<std::uint64_t>(*v);
}

std::optional<Options> parse_args(int argc, char **argv) {
    Options opt;
    for (int i = 1; i < argc; i += 2) {
        const std::string_view flag = argv[i];
        if (i + 1 >= argc) {
            std::fprintf(stderr, "missing value for %s\n", argv[i]);
            return std::nullopt;
        }
        const char *value = argv[i + 1];
        bool ok = true;
        if (flag == "--snr-start" || flag == "--snr-stop" || flag == "--snr-step") {
            const auto v = parse_double(value);
            ok = v.has_value();
            if (ok) {
                (flag == "--snr-start"  ? opt.snr_start_db
                 : flag == "--snr-stop" ? opt.snr_stop_db
                                        : opt.snr_step_db) = *v;
            }
        } else if (flag == "--min-errors" || flag == "--max-bits" || flag == "--seed") {
            const auto v = parse_count(value);
            ok = v.has_value();
            if (ok) {
                (flag == "--min-errors" ? opt.min_errors
                 : flag == "--max-bits" ? opt.max_bits
                                        : opt.seed) = *v;
            }
        } else if (flag == "--out") {
            opt.out = value;
        } else {
            std::fprintf(stderr, "unknown flag %s\n", argv[i]);
            return std::nullopt;
        }
        if (!ok) {
            std::fprintf(stderr, "bad value for %s: %s\n", argv[i], value);
            return std::nullopt;
        }
    }

    if (opt.snr_step_db <= 0.0) {
        std::fputs("--snr-step must be > 0\n", stderr);
        return std::nullopt;
    }
    if (opt.snr_stop_db < opt.snr_start_db) {
        std::fputs("--snr-stop must be >= --snr-start\n", stderr);
        return std::nullopt;
    }
    if (opt.min_errors == 0 || opt.max_bits == 0) {
        std::fputs("--min-errors and --max-bits must be > 0\n", stderr);
        return std::nullopt;
    }
    return opt;
}

}  // namespace

int main(int argc, char **argv) {
    const auto opt = parse_args(argc, argv);
    if (!opt) {
        std::fputs(kUsage.data(), stderr);
        return 2;
    }

    std::FILE *csv = std::fopen(opt->out.c_str(), "w");
    if (csv == nullptr) {
        std::fprintf(stderr, "cannot open %s for writing\n", opt->out.c_str());
        return 1;
    }

    const double ebn0_offset_db = 10.0 * std::log10(2.0);
    std::fprintf(csv, "# ber_sweep: thin slice, uncoded QPSK, OFDM (72 subcarriers), AWGN\n");
    std::fprintf(csv,
                 "# snr_db is SNR per RE (Es/N0 after an unscaled FFT); ebn0_db = snr_db - %.4f\n",
                 ebn0_offset_db);
    std::fprintf(csv, "# seed=%" PRIu64 " min_errors=%" PRIu64 " max_bits=%" PRIu64 "\n", opt->seed,
                 opt->min_errors, opt->max_bits);
    std::fprintf(csv, "snr_db,ebn0_db,bits,errors,ber,capped\n");

    // Point count from an integer index, so repeated float addition cannot drop the last point.
    const auto num_points = static_cast<std::uint64_t>(std::floor(
                                (opt->snr_stop_db - opt->snr_start_db) / opt->snr_step_db + 1e-9)) +
                            1;

    lte::sim::ThinSliceLink link(opt->seed);
    const auto t0 = std::chrono::steady_clock::now();
    for (std::uint64_t i = 0; i < num_points; ++i) {
        const auto snr_f =
            static_cast<float>(opt->snr_start_db + static_cast<double>(i) * opt->snr_step_db);
        const auto snr_db = static_cast<double>(snr_f);  // the value the link actually used

        const lte::sim::ErrorCount count =
            lte::sim::measure_ber(link, snr_f, opt->min_errors, opt->max_bits);
        const double ber = static_cast<double>(count.errors) / static_cast<double>(count.bits);
        const int capped = count.errors < opt->min_errors ? 1 : 0;

        std::fprintf(csv, "%.4f,%.4f,%" PRIu64 ",%" PRIu64 ",%.6e,%d\n", snr_db,
                     snr_db - ebn0_offset_db, count.bits, count.errors, ber, capped);
        std::fflush(csv);

        const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - t0;
        std::fprintf(
            stderr,
            "SNR %6.2f dB  errors %8" PRIu64 " / %12" PRIu64 " bits  BER %.3e%s  (%.1f s)\n",
            snr_db, count.errors, count.bits, ber, capped ? "  [capped]" : "", elapsed.count());
    }

    if (std::fclose(csv) != 0) {
        std::fprintf(stderr, "error writing %s\n", opt->out.c_str());
        return 1;
    }
    return 0;
}
