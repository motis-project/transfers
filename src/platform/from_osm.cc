#include "transfers/platform/from_osm.h"

#include "osmium/handler/node_locations_for_ways.hpp"
#include "osmium/index/map/flex_mem.hpp"

#include "utl/pipes/all.h"
#include "utl/pipes/remove_if.h"
#include "utl/pipes/transform.h"
#include "utl/pipes/unique.h"
#include "utl/pipes/vec.h"
#include "utl/verify.h"

namespace transfers {

std::vector<platform>
osm_platform_extractor::get_platforms_identified_in_osm_file() {
  osmium::index::map::FlexMem<osmium::unsigned_object_id_type, osmium::Location>
      flexmemory;
  osmium::handler::NodeLocationsForWays<typeof(flexmemory)>
      location_to_ways_handler{flexmemory};

  osmium::io::Reader reader{osm_file_, osmium::io::read_meta::no};
  osmium::apply(
      reader, location_to_ways_handler, platform_handler_,
      mp_manager_.handler([this](osmium::memory::Buffer const& area_buffer) {
        osmium::apply(area_buffer, platform_handler_);
      }));
  reader.close();

  return platform_handler_.platforms_;
}

osmium::geom::Coordinates osm_platform_extractor::calc_center(
    const osmium::NodeRefList& ref_list) {
  osmium::geom::Coordinates coord;

  for (auto const& node_ref : ref_list) {
    coord.x += node_ref.lon();
    coord.y += node_ref.lat();
  }

  coord.x /= ref_list.size();
  coord.y /= ref_list.size();

  return coord;
}

void osm_platform_extractor::platform_handler::node(const osmium::Node& node) {
  auto const& tag_list = node.tags();
  if (is_platform(tag_list)) {
    auto const names = get_platform_names(tag_list);
    auto const is_bus_stop = platform_is_bus_stop(tag_list);
    add_platform(osm_type::kNode, node.id(), node.location(), names,
                 is_bus_stop);
  }
}

void osm_platform_extractor::platform_handler::way(osmium::Way const& way) {
  auto const& tag_list = way.tags();
  if (is_platform(tag_list)) {
    auto const names = get_platform_names(tag_list);
    auto const is_bus_stop = platform_is_bus_stop(tag_list);
    add_platform(osm_type::kWay, way.id(), way.envelope().bottom_left(), names,
                 is_bus_stop);
  }
}

void osm_platform_extractor::platform_handler::area(osmium::Area const& area) {
  auto const& tag_list = area.tags();
  if (is_platform(tag_list)) {
    auto const coord = calc_center(*area.cbegin<osmium::OuterRing>());
    auto const names = get_platform_names(tag_list);
    auto const is_bus_stop = platform_is_bus_stop(tag_list);
    add_platform(area.from_way() ? osm_type::kWay : osm_type::kRelation,
                 area.orig_id(), coord, names, is_bus_stop);
  }
}

void osm_platform_extractor::platform_handler::add_platform(
    osm_type const type, osmium::object_id_type const id,
    osmium::geom::Coordinates const& coord, vector<string_t> const& names,
    bool is_bus_stop) {
  platforms_.emplace_back(
      platform{geo::latlng{coord.y, coord.x}, id, type, names, is_bus_stop});
}

bool osm_platform_extractor::platform_handler::is_platform(
    osmium::TagList const& tags) const {
  return osmium::tags::match_any_of(tags, filter_);
}

bool osm_platform_extractor::platform_handler::platform_is_bus_stop(
    osmium::TagList const& tag_list) {
  return (tag_list.has_tag("highway", "bus_stop"));
}

vector<string_t> osm_platform_extractor::platform_handler::get_platform_names(
    osmium::TagList const& tag_list) {
  auto const default_value = string_t{"n/a"};

  auto vector_names =
      utl::all(name_tags_) |
      utl::transform([&tag_list, &default_value](auto const& key) {
        return string_t{tag_list.get_value_by_key(key.c_str(),
                                                  default_value.str().c_str())};
      }) |
      utl::unique() | utl::remove_if([&default_value](auto const& name) {
        return name == default_value;
      }) |
      utl::vec();

  auto names = vector<string_t>{};
  for (auto const& name : vector_names) {
    names.emplace_back(name);
  }

  return names;
}

}  // namespace transfers
