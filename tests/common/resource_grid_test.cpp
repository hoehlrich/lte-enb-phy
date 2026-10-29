#include "lte/common/resource_grid.hpp"

#include <cstddef>
#include <span>

#include <gtest/gtest.h>

#include "golden/compare.hpp"
#include "golden/golden.hpp"
#include "lte/common/constants.hpp"
#include "lte/common/types.hpp"

TEST(ResourceGrid, LayoutMatchesMatlab) {
    const auto ref = golden::load<lte::cf32>("common/resource_grid");

    lte::ResourceGrid grid{};
    for (std::size_t l = 0; l < lte::kSymbolsPerSubframe; ++l) {
        auto sym = grid.symbol(l);
        for (std::size_t k = 0; k < lte::kNumSubcarriers; ++k) {
            sym[k] = {float(k + 1), float(l + 1)};  // MATLAB is 1-based
        }
    }

    const std::span<const lte::cf32> flat = ref.data;
    for (std::size_t l = 0; l < lte::kSymbolsPerSubframe; ++l) {
        EXPECT_TRUE(golden::near(
            grid.symbol(l), flat.subspan(l * lte::kNumSubcarriers, lte::kNumSubcarriers), 1e-6))
            << "symbol " << l;
    }
}

TEST(ResourceGridDeathTest, SymbolIndexOutOfRangeAborts) {
    lte::ResourceGrid grid{};
    EXPECT_DEATH(grid.symbol(lte::kSymbolsPerSubframe), "kSymbolsPerSubframe");
}
