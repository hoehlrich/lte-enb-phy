add_library(ltephy STATIC
    lte/mod/qpsk.cpp
)

target_include_directories(ltephy PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ltephy PUBLIC PkgConfig::FFTW3F)
lte_enable_warnings(ltephy)
