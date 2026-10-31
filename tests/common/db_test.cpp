#include "lte/common/db.hpp"

#include <initializer_list>

#include <gtest/gtest.h>

TEST(Db, KnownPowerRatios) {
    EXPECT_FLOAT_EQ(lte::db_to_power_ratio(0.0f), 1.0f);
    EXPECT_FLOAT_EQ(lte::db_to_power_ratio(10.0f), 10.0f);
    EXPECT_FLOAT_EQ(lte::db_to_power_ratio(-20.0f), 0.01f);
    // Power, not amplitude: 3 dB is about 2x (amplitude would be about 1.41x).
    EXPECT_NEAR(lte::db_to_power_ratio(3.0f), 1.995f, 1e-3f);
}

TEST(Db, KnownDecibels) {
    EXPECT_FLOAT_EQ(lte::power_ratio_to_db(1.0f), 0.0f);
    EXPECT_FLOAT_EQ(lte::power_ratio_to_db(100.0f), 20.0f);
    EXPECT_NEAR(lte::power_ratio_to_db(128.0f / 72.0f), 2.499f, 1e-3f);  // the occupancy offset
}

TEST(Db, RoundTrip) {
    for (const float db : {-30.0f, -5.5f, 0.0f, 2.5f, 17.0f, 40.0f}) {
        EXPECT_NEAR(lte::power_ratio_to_db(lte::db_to_power_ratio(db)), db, 1e-4f) << db << " dB";
    }
}

TEST(DbDeathTest, NonPositiveRatioAborts) {
    EXPECT_DEATH(lte::power_ratio_to_db(0.0f), "ratio > 0");
    EXPECT_DEATH(lte::power_ratio_to_db(-1.0f), "ratio > 0");
}
