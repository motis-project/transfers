#include "gtest/gtest.h"

#include "transfers/types.h"

TEST(types, location) {
  using namespace transfers;
  constexpr auto kLatMin = -180;
  constexpr auto kLatMax = 180;
  constexpr auto kLonMin = -90;
  constexpr auto kLonMax = 90;
  constexpr auto kTestPrecision = 100;

  auto tested_coords = 0U;

  auto vrfy_location = [](float const lat, float const lon) {
    auto const loc = location(lat, lon);
    ASSERT_EQ(location(loc.key()), loc);
  };

  for (auto lat = kLatMin * kTestPrecision; lat <= kLatMax * kTestPrecision;
       ++lat) {
    for (auto lon = kLonMin * kTestPrecision; lon <= kLonMax * kTestPrecision;
         ++lon) {
      vrfy_location(static_cast<float>(lat) / kTestPrecision,
                    static_cast<float>(lon) / kTestPrecision);
      ++tested_coords;
    }
  }

  std::clog << "Tested " << tested_coords << " unique coordinates.\n";
}