# lte-enb-phy

A tower-side (eNodeB) LTE PHY slice in C++20, verified bit-exact against MATLAB LTE Toolbox.

Goals:
- **Correct:** every block matches LTE Toolbox golden vectors (3GPP Release 10).
- **Measured:** uplink BLER against SNR, frequency offset, and timing offset.
- **Fast:** a fixed-point, NEON-optimized turbo decoder profiled on ARM.

## Build

Requires CMake ≥ 3.25, Ninja, FFTW (single precision), and GCC or Clang.

```sh
cmake --workflow --preset debug     # ASan + UBSan, for correctness
cmake --workflow --preset release   # optimized, for sweeps
```

## Layout (planned)

```
src/lte/     the library: common, ofdm, mod, coding, ul, dl, rx
sim/         test-bench channel: AWGN, frequency offset, timing offset
apps/        sweep drivers and hardware tools
tests/       unit tests against golden vectors
matlab/      LTE Toolbox models that generate the vectors
vectors/     committed golden vectors (.bin + .json)
scripts/     plotting
```
