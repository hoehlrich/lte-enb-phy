// Proves the toolchain end to end: C++20, GoogleTest, FFTW linkage, sanitizers.
// Delete once the OFDM tests exist.
#include <vector>

#include <fftw3.h>
#include <gtest/gtest.h>

#include "lte/common/types.hpp"

TEST(Smoke, FftOfImpulseIsFlat) {
    constexpr int n = 8;
    std::vector<lte::cf32> in(n), out(n);
    in[0] = {1.0f, 0.0f};

    fftwf_plan plan = fftwf_plan_dft_1d(n, reinterpret_cast<fftwf_complex *>(in.data()),
                                        reinterpret_cast<fftwf_complex *>(out.data()), FFTW_FORWARD,
                                        FFTW_ESTIMATE);
    ASSERT_NE(plan, nullptr);
    fftwf_execute(plan);
    fftwf_destroy_plan(plan);

    for (const auto &x : out) {
        EXPECT_FLOAT_EQ(x.real(), 1.0f);
        EXPECT_FLOAT_EQ(x.imag(), 0.0f);
    }
}
