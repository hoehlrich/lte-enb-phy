#pragma once

#include <cstdio>
#include <cstdlib>

namespace lte::detail {

[[noreturn]] inline void contract_fail(const char *cond, const char *file, int line) {
    std::fprintf(stderr, "contract violated: %s at %s:%d\n", cond, file, line);
    std::abort();
}

}  // namespace lte::detail

// Custom precondition check so it stays in release builds
#define LTE_EXPECTS(cond)                                                     \
    do {                                                                      \
        if (!(cond)) ::lte::detail::contract_fail(#cond, __FILE__, __LINE__); \
    } while (0)
