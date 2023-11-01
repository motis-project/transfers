#pragma once

#include <vector>

#include "transfers/platform/platform.h"
#include "transfers/platform/platform_index.h"
#include "transfers/types.h"

#include "nigiri/timetable.h"

namespace transfers {

struct matching_data {
  ::nigiri::timetable::locations const& locations_to_match_;

  hash_map<location_key_t, platform> const& already_matched_nloc_keys_;

  platform_index const& old_state_pf_idx_;
  platform_index const& update_state_pf_idx_;

  bool has_update_state_pf_idx_;
};

struct matching_options {
  double max_matching_dist_;
  double max_bus_stop_matching_dist_;
};

struct matching_result {
  platform pf_;
  location loc_;
};

struct matcher {

  explicit matcher(matching_data const& data, matching_options const& options)
      : data_(data), options_(options) {}

  virtual ~matcher() = default;

  matcher(const matcher&) = delete;
  matcher& operator=(const matcher&) = delete;

  matcher(matcher&&) = delete;
  matcher& operator=(matcher&&) = delete;

  // Matches `nigiri::location`s with `platform`s extracted from OSM data and
  // returns a list of valid matches.
  virtual std::vector<matching_result> matching() = 0;

  matching_data const data_;
  matching_options const options_;
};

}  // namespace transfers
