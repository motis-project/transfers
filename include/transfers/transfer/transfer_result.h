#pragma once

#include <ostream>
#include <vector>

#include "transfers/transfer/transfer_request.h"
#include "transfers/types.h"

#include "ppr/common/routing_graph.h"
#include "ppr/routing/routing_query.h"
#include "ppr/routing/search_profile.h"

#include "nigiri/types.h"

namespace transfers {

struct transfer_info {
  CISTA_COMPARABLE();
  friend std::ostream& operator<<(std::ostream&, transfer_info const&);

  ::nigiri::duration_t duration_{};
  double distance_{};
};

struct transfer_result {
  CISTA_COMPARABLE();
  friend std::ostream& operator<<(std::ostream&, transfer_result const&);

  // Returns a short and unique `transfer_result` representation that can be
  // used as a database id/key.
  string_t key() const;

  nlocation_key_t from_nloc_key_;
  vector<nlocation_key_t> to_nloc_keys_;
  profile_key_t profile_;

  vector<transfer_info> infos_;
};

// Builds a ppr::routing_query using the given `transfer_request` and a map of
// `profile_keys_t` to `search_profile_` to get the search profile of the
// `transfer_request`.
::ppr::routing::routing_query build_routing_query(
    const hash_map<profile_key_t,
                   ::ppr::routing::search_profile>& /* profiles */,
    transfer_request const& /* treq */);

// Routes a single `transfer_request` and returns the corresponding
// `transfer_result`. PPR is used for routing.
transfer_result route_single_request(
    transfer_request const&, ::ppr::routing_graph const&,
    hash_map<profile_key_t, ::ppr::routing::search_profile> const&);

// Routes a batch of `transfer_request`s and returns the corresponding list of
// `transfer_result`s.
// Equivalent: Calls `route_single_request` for every
// `transfer_request`.
std::vector<transfer_result> route_multiple_requests(
    std::vector<transfer_request> const&, ::ppr::routing_graph const&,
    hash_map<profile_key_t, ::ppr::routing::search_profile> const&);

// Returns a new merged `transfer_result` struct.
// Default values used from `lhs` struct.
// Adds `to_nloc_keys_` and corresponding `info` if the `to_nloc_key` is not yet
// considered over `lhs`.
// Limitations:
// - Already existing `transfer_result::info` in `lhs` will not be updated if
// there is a newer one in `rhs`.
// Merge Prerequisites:
// - `lhs.from_nloc_key_ == rhs.from_nloc_key_`
// - `lhs.profile_ == rhs.profile_`
// - `lhs.to_nloc_keys_.size() == lhs.infos_.size()`
// - `rhs.to_nloc_keys_.size() == rhs.infos_.size()`
transfer_result merge(transfer_result const& /* a */,
                      transfer_result const& /* b */);

}  // namespace transfers
