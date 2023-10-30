#include "transfers/platform/extract.h"

#include <string>

#include "transfers/platform/from_osm.h"

#include "osmium/tags/tags_filter.hpp"

namespace fs = std::filesystem;

namespace transfers {

// Adds a set of filter rules according to which the platforms should be
// extracted from the OSM file.
// Returns the generated TagsFilter.
osmium::TagsFilter get_tags_filter() {
  auto filter = osmium::TagsFilter{false};
  filter.add_rule(true, "public_transport", "platform");
  filter.add_rule(true, "public_transport", "stop_position");
  filter.add_rule(true, "railway", "platform");
  filter.add_rule(true, "railway", "tram_stop");
  return filter;
}

// Returns a list of keys that will be searched for in the tag list for names of
// the current platform.
std::vector<std::string> get_name_tags() {
  return {"name", "description", "ref_name", "local_ref", "ref"};
}

std::vector<platform> extract_platforms_from_osm_file(
    fs::path const& osm_file_path) {
  auto osm_extractor =
      osm_platform_extractor(osm_file_path, get_tags_filter(), get_name_tags());

  return osm_extractor.get_platforms_identified_in_osm_file();
}

}  // namespace transfers
