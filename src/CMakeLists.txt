# Header-only until the first .cpp lands. Then:
#   add_library(ltephy STATIC lte/.../x.cpp)
#   change INTERFACE -> PUBLIC below
#   lte_enable_warnings(ltephy)
add_library(ltephy INTERFACE)
target_include_directories(ltephy INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ltephy INTERFACE PkgConfig::FFTW3F)
