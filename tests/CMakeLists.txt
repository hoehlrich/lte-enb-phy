include(FetchContent)
FetchContent_Declare(googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.18.0
  GIT_SHALLOW    TRUE
  SYSTEM         # GoogleTest headers don't trip our warnings
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK   OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

# lte_add_test(<name> <sources...>): one test executable, linked to the library.
function(lte_add_test name)
  add_executable(${name} ${ARGN})
  target_link_libraries(${name} PRIVATE ltephy GTest::gtest_main)
  lte_enable_warnings(${name})
  # PRE_TEST: list tests when ctest runs, not at build time. Required once we
  # cross-compile for the ARM board, where the build host can't run the binary.
  gtest_discover_tests(${name} DISCOVERY_MODE PRE_TEST)
endfunction()

lte_add_test(smoke_test smoke_test.cpp)
