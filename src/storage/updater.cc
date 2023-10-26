#include "transfers/storage/updater.h"

#include "transfers/matching/by_distance.h"
#include "transfers/platform/extract.h"
#include "transfers/transfer/transfer_request.h"
#include "transfers/transfer/transfer_result.h"

#include "ppr/common/routing_graph.h"
#include "ppr/serialization/reader.h"

namespace p = ::ppr;
namespace ps = ::ppr::serialization;

namespace transfers {

void storage_updater::full_update() {
  // 1st: extract all platforms from a given osm file
  extract_and_store_osm_platforms();

  // 2nd: update osm_id and location_idx: match osm and nigiri locations
  match_and_store_matches_by_distance();

  // 3rd: generate transfer requests
  generate_and_store_transfer_requests(false);

  // 4th: precompute transfers (generate transfer results)
  generate_and_store_transfer_results();

  // 5th: update timetable
  storage_.update_tt(storage_.nigiri_dump_path_);
}

void storage_updater::extract_and_store_osm_platforms() {
  storage_.add_new_platforms(
      std::move(extract_platforms_from_osm_file(storage_.osm_path_)));
}

void storage_updater::match_and_store_matches_by_distance() {
  auto matcher = distance_matcher(
      storage_.get_matching_data(),
      {storage_.max_matching_dist_, storage_.max_bus_stop_matching_dist_});
  storage_.add_new_matching_results(std::move(matcher.matching()));
}

void storage_updater::generate_and_store_transfer_requests(
    bool const old_to_old) {
  storage_.add_new_transfer_requests_keys(
      std::move(generate_transfer_requests_keys(
          storage_.get_transfer_request_keys_generation_data(),
          {.old_to_old_ = old_to_old})));
}

void storage_updater::generate_and_store_transfer_results() {
  p::routing_graph rg;
  ps::read_routing_graph(rg, storage_.ppr_rg_path_);

  auto treqs = to_transfer_requests(
      storage_.get_transfer_requests_keys(data_request_type::kPartialUpdate),
      storage_.get_all_matchings());

  storage_.add_new_transfer_results(std::move(route_multiple_requests(
      treqs, rg, storage_.profile_key_to_search_profile_)));
}

}  // namespace transfers
