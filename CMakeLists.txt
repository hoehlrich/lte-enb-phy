cmake_minimum_required(VERSION 3.25)
project(lte-enb-phy VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(Sanitizers)  # global flags: must come before any target, including GoogleTest
include(Warnings)    # defines lte_enable_warnings(<target>)

# pkg-config rather than find_package(FFTW3): the CMake config files shipped by
# distro FFTW packages (Arch, Debian/Ubuntu) are incomplete and fail to load.
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFTW3F REQUIRED IMPORTED_TARGET fftw3f)

add_subdirectory(src)

enable_testing()
add_subdirectory(tests)
