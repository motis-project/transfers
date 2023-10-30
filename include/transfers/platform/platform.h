#pragma once

#include <cstdint>
#include <vector>

#include "transfers/types.h"

#include "geo/latlng.h"

namespace transfers {

enum class osm_type { kNode, kWay, kRelation, kUnknown };

// Returns the char representation of the given `osm_type`.
// n: kNode, w: kWay, r: kRelation, u: kUnknown
char get_osm_type_as_char(osm_type const);

struct platform {
  geo::latlng loc_;
  std::int64_t osm_id_{-1};
  osm_type osm_type_{osm_type::kNode};
  vector<string_t> names_;
  bool is_bus_stop_{false};
};

// Returns a string representation of the given platform.
string_t to_key(platform const&);

// platform equal operator
bool operator==(platform const& a, platform const& b);

}  // namespace transfers
