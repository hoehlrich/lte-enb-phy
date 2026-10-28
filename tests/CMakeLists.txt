include(FetchContent)
FetchContent_Declare(googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.18.0
  GIT_SHALLOW    TRUE
  SYSTEM         # GoogleTest headers don't trip our warnings
)
FetchContent_Declare(json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.12.0
  GIT_SHALLOW    TRUE
  SYSTEM
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK   OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest json)

include(GoogleTest)

# Golden-vector loader: test-only
add_library(golden STATIC golden/golden.cpp golden/compare.cpp)
target_include_directories(golden PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(golden PUBLIC ltephy nlohmann_json::nlohmann_json GTest::gtest)
target_compile_definitions(golden PRIVATE LTE_VECTOR_DIR="${PROJECT_SOURCE_DIR}/vectors")
lte_enable_warnings(golden)

# lte_add_test(<name> <sources...>): one test executable, linked to the library.
function(lte_add_test name)
  add_executable(${name} ${ARGN})
  target_link_libraries(${name} PRIVATE ltephy golden GTest::gtest_main)
  lte_enable_warnings(${name})
  # PRE_TEST: list tests when ctest runs, not at build time. Required once we
  # cross-compile for the ARM board, where the build host can't run the binary.
  gtest_discover_tests(${name} DISCOVERY_MODE PRE_TEST)
endfunction()

lte_add_test(smoke_test smoke_test.cpp)
lte_add_test(golden_test golden/golden_test.cpp)
lte_add_test(compare_test golden/compare_test.cpp)
lte_add_test(qpsk_test mod/qpsk_test.cpp)
