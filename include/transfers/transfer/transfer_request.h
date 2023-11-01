#pragma once

#include <ostream>
#include <vector>

#include "transfers/platform/platform.h"
#include "transfers/platform/platform_index.h"
#include "transfers/types.h"

#include "ppr/routing/search_profile.h"

namespace transfers {

struct transfer_request_by_keys {
  CISTA_COMPARABLE()
  friend std::ostream& operator<<(std::ostream&,
                                  transfer_request_by_keys const&);

  // Returns a short and unique `transfer_request_by_keys` representation that
  // can be used as a database id/key.
  string_t key() const;

  location_key_t from_loc_;
  vector<location_key_t> to_locs_;
  profile_key_t profile_;
};

struct transfer_request {
  friend std::ostream& operator<<(std::ostream&, transfer_request const&);

  // Returns a short and unique `transfer_request` representation that can be
  // used as a database id/key.
  string_t key() const;

  platform transfer_start_;
  location from_loc_;

  std::vector<platform> transfer_targets_;
  vector<location> to_locs_;

  profile_key_t profile_;
};

struct transfer_request_generation_data {
  struct matched_nigiri_location_data {
    platform_index const& matched_pfs_idx_;
    vector<location>& locs_;
    bool set_matched_pfs_idx_;
  } old_, update_;

  hash_map<profile_key_t, ::ppr::routing::search_profile>
      profile_key_to_search_profile_;
};

struct transfer_request_options {
  // old_to_old: build transfer requests from already processed (matched
  // platforms) in old_state; use if profiles_hash has been changed
  bool old_to_old_;
};

// Creates a list of `transfer_request` struct from the given list of
// `transfer_request_keys` struct and returns it. Keys are replaced by the
// matched platform.
// Requirement: X must only contain nigiri locations that have
// been successfully matched to an OSM platform.
std::vector<transfer_request> to_transfer_requests(
    std::vector<transfer_request_by_keys> const&,
    hash_map<location_key_t, platform> const&);

// Generates new `transfer_request_keys` based on matched platforms in the old
// and update state. List of `transfer_request_keys` are always created for the
// following combinations:
// - from old (known matches) to update (new matches)
// - from update (new matches) to old (old matches)
// - from update (new matches) to update (new matches)
// Depending on the given options, the additional creation for the following
// combination is also possible:
// - from old (known matches) to old (known matches)
// The last combination is usually ignored, because the resulting list of
// `transfer_request_keys` are already stored in the storage. An update of these
// `transfer_request_keys` is necessary if the profiles (from PPR) necessary for
// the creation of the `transfer_request_keys` have changed (compared to
// previous applications).
std::vector<transfer_request_by_keys>
generate_all_pair_transfer_requests_by_keys(
    transfer_request_generation_data const&, transfer_request_options const&);

// Returns the new merged `transfer_request_keys` struct.
// Default values used from `a` struct.
// Adds `to_nloc_keys_` from `rhs` if `nloc_key` is not yet considered over.
// `a`.
// Merge Prerequisites:
// - both `transfer_request_keys` structs have the same `from_nloc_key_`
// - both `transfer_request_keys` structs have the same `profile_key_t`
transfer_request_by_keys merge(transfer_request_by_keys const& /* a */,
                               transfer_request_by_keys const& /* b */);

}  // namespace transfers
