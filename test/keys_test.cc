#include "gtest/gtest.h"

#include "transfers/types.h"

TEST(types, location) {
  using namespace transfers;
  constexpr auto kLatMin = -180.;
  constexpr auto kLatMax = 180.;
  constexpr auto kLngMin = -90.;
  constexpr auto kLngMax = 90.;
  constexpr auto kTestPrecision = 0.01;

  auto tested_coords = 0U;

  auto vrfy_location = [](double const lat, double const lon) {
    auto const loc = location(lat, lon);
    ASSERT_EQ(location(loc.key()), loc);
  };

  for (auto lat = kLatMin; lat <= kLatMax;) {
    for (auto lng = kLngMin; lng <= kLngMax;) {
      vrfy_location(lat, lng);
      ++tested_coords;
      lng += kTestPrecision;
    }
    lat += kTestPrecision;
  }

  // std::clog << "Tested " << tested_coords << " coordinates.\n";
}
