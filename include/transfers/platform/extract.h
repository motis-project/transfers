#pragma once

#include <filesystem>
#include <tuple>
#include <vector>

#include "transfers/platform/platform.h"

namespace transfers {

// Returns a list of `platform`s extracted from the given osm file.
// To identify platforms the defined filter rules (`filter_rule_descriptions`)
// are applied.
// To extract the names of a platform the keys set in `platform_name_keys` are
// used.
std::vector<platform> extract_platforms_from_osm_file(
    std::filesystem::path const&);

}  // namespace transfers
