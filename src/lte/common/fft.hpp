#pragma once

#include <cstddef>
#include <memory>
#include <span>

#include "lte/common/types.hpp"

// Forward declaration of FFTW's plan type, so including this header doesn't pull in <fftw3.h>.
struct fftwf_plan_s;

namespace lte {

// One planned, fixed-size complex FFT (FFTW, single precision) with its own aligned buffers.
//
// Plan once, execute many times: construction is the expensive step (FFTW_MEASURE times
// several algorithms), execute() is cheap. Typical use per transform:
//
//     std::ranges::copy(input, fft.in().begin());
//     fft.execute();
//     ... read fft.out() ...
//
// Unnormalized, like FFTW itself: an inverse followed by a forward multiplies by size().
// Not copyable (it owns a plan and buffers); movable.
class Fft {
public:
    enum class Direction { kForward, kInverse };

    Fft(std::size_t n, Direction dir);

    std::span<cf32> in() { return {in_.get(), n_}; }
    std::span<const cf32> in() const { return {in_.get(), n_}; }
    std::span<const cf32> out() const { return {out_.get(), n_}; }

    void execute();

    std::size_t size() const { return n_; }

private:
    struct BufferDeleter {
        void operator()(cf32 *p) const;
    };
    struct PlanDeleter {
        void operator()(fftwf_plan_s *p) const;
    };

    // Declaration order matters: members are destroyed in reverse, so the plan goes first.
    std::size_t n_;
    std::unique_ptr<cf32[], BufferDeleter> in_;
    std::unique_ptr<cf32[], BufferDeleter> out_;
    std::unique_ptr<fftwf_plan_s, PlanDeleter> plan_;
};

}  // namespace lte
