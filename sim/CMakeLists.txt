# ltesim: the test-bench channel (noise, and later frequency and timing offest).
# Kept out of ltephy so the receiver can never see what the channel did
add_library(ltesim STATIC
    lte/sim/awgn.cpp
)
target_include_directories(ltesim PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ltesim PUBLIC ltephy)
lte_enable_warnings(ltesim)
