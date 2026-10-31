# lte-enb-phy

A tower-side (eNodeB) LTE PHY slice in C++20, verified bit-exact against MATLAB LTE Toolbox.

## Build

Requires CMake ≥ 3.25, Ninja, FFTW (single precision), and GCC or Clang.

```sh
cmake --workflow --preset debug     # ASan + UBSan, for correctness
cmake --workflow --preset release   # optimized, for sweeps
