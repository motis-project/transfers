#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <bit>
#include <limits>

#include "ankerl/cista_adapter.h"

#include "cista/containers/array.h"
#include "cista/containers/mutable_fws_multimap.h"
#include "cista/containers/string.h"
#include "cista/containers/vector.h"
#include "cista/hash.h"

#include "geo/latlng.h"

namespace transfers {

constexpr auto kCoordinatePrecision = 10000000.0;

template <typename K, typename V, typename Hash = cista::hash_all,
          typename Equality = cista::equals_all>
using hash_map = cista::raw::ankerl_map<K, V, Hash, Equality>;

template <typename K, typename Hash = cista::hash_all,
          typename Equality = cista::equals_all>
using set = cista::raw::ankerl_set<K, Hash, Equality>;

template <typename K, typename V>
using mutable_fws_multimap = cista::raw::mutable_fws_multimap<K, V>;

template <typename V, std::size_t SIZE>
using array = cista::raw::array<V, SIZE>;

template <typename V>
using vector = cista::offset::vector<V>;

// using nlocation_key_t = std::uint64_t;
using location_key_t = std::int64_t;
using profile_key_t = std::uint8_t;

using string_t = cista::offset::string;

struct location {
  bool operator==(location const& other) const {
    return latlng_.latlng_full_ == other.latlng_.latlng_full_;
  }

  location() = default;
  explicit location(geo::latlng const& coord) {
    latlng_.split_.lat_ = double_to_fix(coord.lat_);
    latlng_.split_.lng_ = double_to_fix(coord.lng_);
  }

  location(double lat, double lng) {
    latlng_.split_.lat_ = double_to_fix(lat);
    latlng_.split_.lng_ = double_to_fix(lng);
  }

  explicit location(location_key_t key) { latlng_.latlng_full_ = key; }

  static std::int32_t double_to_fix(double const d) {
    return static_cast<std::int32_t>(std::round(d * kCoordinatePrecision));
  }

  static float fix_to_double(std::int32_t const i) {
    return static_cast<double>(i) / kCoordinatePrecision;
  }

  // Returns a unique  nigiri location coordinate representation
  // Interprets longitude and latitude as 32b float values. Appends them to
  // each other and thus creates a 64b long coordinate representation.
  // latitude (as 32b) || longitude (as 32b)
  location_key_t key() const { return latlng_.latlng_full_; }

  geo::latlng to_latlng() const {
    return {fix_to_double(latlng_.split_.lat_),
            fix_to_double(latlng_.split_.lng_)};
  }

  union {
    struct {
      std::uint32_t lng_;
      std::uint32_t lat_;
    } split_;
    location_key_t latlng_full_;
  } latlng_{};
};

}  // namespace transfers
