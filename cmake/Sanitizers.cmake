# Runtime checks for the debug preset. Applied globally (including GoogleTest)
# so every object in a sanitized binary is built the same way.
option(LTE_SANITIZE "ASan + UBSan + libstdc++ bounds assertions" OFF)

if(LTE_SANITIZE)
  add_compile_options(
    -fsanitize=address,undefined
    -fno-sanitize-recover=all  # abort on the first UB report, so a test fails instead of printing and passing
    -fno-omit-frame-pointer
  )
  add_link_options(-fsanitize=address,undefined)

  # Bounds-checks std::vector::operator[] and friends. ASan misses an index past
  # size() that is still inside capacity(); this catches it.
  add_compile_definitions(_GLIBCXX_ASSERTIONS)
endif()
