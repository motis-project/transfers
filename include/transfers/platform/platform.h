#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "transfers/types.h"

#include "geo/latlng.h"

namespace transfers {

enum class osm_type { kNode, kWay, kRelation, kUnknown };

// Returns the char representation of the given `osm_type`.
// n: kNode, w: kWay, r: kRelation, u: kUnknown
char get_osm_type_as_char(osm_type const);

struct platform {
  bool operator==(platform const& other) const;

  // Returns a short and unique `platform` representation that can be used as a
  // database id/key.
  std::string key() const;

  geo::latlng loc_;
  std::int64_t osm_id_{-1};
  osm_type osm_type_{osm_type::kNode};
  vector<string_t> names_;
  bool is_bus_stop_{false};
};

}  // namespace transfers
