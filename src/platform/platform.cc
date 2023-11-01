#include "transfers/platform/platform.h"

#include <cstring>

namespace transfers {

char get_osm_type_as_char(osm_type const type) {
  switch (type) {
    case osm_type::kNode: return 'n';
    case osm_type::kWay: return 'w';
    case osm_type::kRelation: return 'r';
    case osm_type::kUnknown: return 'u';
    default: return '_';
  }
}

std::string platform::key() const {
  auto key = std::string{};

  // platform key: osm_type + osm_id
  key.resize(sizeof(osm_type_) + sizeof(osm_id_));
  std::memcpy(key.data(), &osm_type_, sizeof(osm_type_));
  std::memcpy(key.data() + sizeof(osm_type_), &osm_id_, sizeof(osm_id_));

  return key;
}

bool platform::operator==(platform const& other) const {
  return osm_id_ == other.osm_id_ && osm_type_ == other.osm_type_;
};

}  // namespace transfers
