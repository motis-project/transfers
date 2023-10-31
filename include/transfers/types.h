#pragma once

#include <cstddef>
#include <cstdint>
#include <bit>

#include "ankerl/cista_adapter.h"

#include "cista/containers/array.h"
#include "cista/containers/mutable_fws_multimap.h"
#include "cista/containers/string.h"
#include "cista/containers/vector.h"
#include "cista/hash.h"

#include "geo/latlng.h"

namespace transfers {

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
using location_key_t = std::uint64_t;
using profile_key_t = std::uint8_t;

using string_t = cista::offset::string;

using latlng_split = union {
  struct {
    std::uint32_t lng_;
    std::uint32_t lat_;
  } split_;
  location_key_t latlng_full_;
};

struct location {
  bool operator==(location const& other) const { return key() == other.key(); };

  location() = default;
  explicit location(geo::latlng const& coord)
      : lat_(coord.lat_), lng_(coord.lng_) {}
  location(float lat, float lng) : lat_(lat), lng_(lng) {}
  explicit location(location_key_t key) {
    auto latlng_splitted = latlng_split{};
    latlng_splitted.latlng_full_ = key;

    lng_ = std::bit_cast<float>(latlng_splitted.split_.lng_);
    lat_ = std::bit_cast<float>(latlng_splitted.split_.lat_);
  }

  // Returns a unique  nigiri location coordinate representation
  // Interprets longitude and latitude as 32b float values. Appends them to each
  // other and thus creates a 64b long coordinate representation.
  // latitude (as 32b) || longitude (as 32b)
  location_key_t key() const {
    auto const lat32b = std::bit_cast<std::uint32_t>(lat_);
    auto const lng32b = std::bit_cast<std::uint32_t>(lng_);

    return (static_cast<location_key_t>(lat32b) << 32) | lng32b;
  };

  geo::latlng to_latlng() const { return {lat_, lng_}; };

  float lat_{0.0}, lng_{0.0};
};

}  // namespace transfers
