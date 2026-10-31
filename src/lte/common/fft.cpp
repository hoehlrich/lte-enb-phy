#include "lte/common/fft.hpp"

#include <algorithm>
#include <cstddef>

#include <fftw3.h>

#include "lte/common/contract.hpp"
#include "lte/common/types.hpp"

namespace lte {
namespace {

// std::complex<float> is layout-compatible with fftwf_complex (float[2]); FFTW's manual
// documents this cast as safe.
fftwf_complex *as_fftw(cf32 *p) { return reinterpret_cast<fftwf_complex *>(p); }

cf32 *alloc_buffer(std::size_t n) {
    // fftwf_malloc gives SIMD-aligned memory, which lets FFTW use its vectorized kernels.
    auto *p = static_cast<cf32 *>(fftwf_malloc(n * sizeof(cf32)));
    LTE_EXPECTS(p != nullptr);
    return p;
}

}  // namespace

void Fft::BufferDeleter::operator()(cf32 *p) const { fftwf_free(p); }
void Fft::PlanDeleter::operator()(fftwf_plan_s *p) const { fftwf_destroy_plan(p); }

Fft::Fft(std::size_t n, Direction dir) : n_(n), in_(alloc_buffer(n)), out_(alloc_buffer(n)) {
    LTE_EXPECTS(n > 0);

    // FFTW_MEASURE overwrites the buffers while it times candidate algorithms, so plan
    // first and zero afterwards. The planner is not thread-safe; create plans on one thread.
    const int sign = (dir == Direction::kForward) ? FFTW_FORWARD : FFTW_BACKWARD;
    plan_.reset(fftwf_plan_dft_1d(static_cast<int>(n), as_fftw(in_.get()), as_fftw(out_.get()),
                                  sign, FFTW_MEASURE));
    LTE_EXPECTS(plan_ != nullptr);

    std::fill_n(in_.get(), n_, cf32{});
    std::fill_n(out_.get(), n_, cf32{});
}

void Fft::execute() {
    LTE_EXPECTS(plan_ != nullptr);  // catches use after move
    fftwf_execute(plan_.get());     // safe to call from several threads on different plans
}

}  // namespace lte
