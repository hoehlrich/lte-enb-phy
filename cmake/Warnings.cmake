# lte_enable_warnings(<target>): strict warnings, treated as errors.
# Per target, so third-party code (GoogleTest) isn't held to these rules.
# To build past warnings while sketching:
#   cmake --preset debug --compile-no-warning-as-error
function(lte_enable_warnings target)
  target_compile_options(${target} PRIVATE
    -Wall -Wextra -Wpedantic
    -Wshadow
    -Wconversion -Wsign-conversion  # silent narrowing is the classic fixed-point bug
    -Wdouble-promotion              # keep float pipelines in float
    -Wold-style-cast
    -Wnon-virtual-dtor -Woverloaded-virtual
    -Wimplicit-fallthrough
    -Wformat=2
  )
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${target} PRIVATE
      -Wduplicated-cond -Wduplicated-branches -Wlogical-op
    )
  endif()
  set_target_properties(${target} PROPERTIES COMPILE_WARNING_AS_ERROR ON)
endfunction()
