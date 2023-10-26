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
  generate_and_store_transfer_requests();

  // 4th: precompute transfers (generate transfer results)
  generate_and_store_transfer_results(data_request_type::kPartialUpdate);

  // 5th: update timetable
  storage_.update_tt(storage_.nigiri_dump_path_);
}

void storage_updater::partial_update(transfers::first_update const first,
                                     transfers::routing_type const routing) {
  // 1st: extract all platforms from a given osm file
  // 2nd: update osm_id and location_idx: match osm and nigiri locations
  // 3rd: generate transfer requests
  switch (first) {
    case first_update::kNoUpdate: break;
    case first_update::kOSM: extract_and_store_osm_platforms();
    case first_update::kTimetable:
      match_and_store_matches_by_distance();
      generate_and_store_transfer_requests();
      break;
    case first_update::kProfiles:
      generate_and_store_transfer_requests(true);
      break;
  }

  // 4th: precompute transfers (generate transfer results)
  switch (routing) {
    case routing_type::kNoRouting: break;
    case routing_type::kPartialRouting:
      // do not load ppr graph if there are no routing requests
      if (storage_.has_transfer_requests_keys(
              data_request_type::kPartialUpdate)) {
        break;
      }
      // route "new" transfer requests
      generate_and_store_transfer_results(data_request_type::kPartialUpdate);
      break;
    case routing_type::kFullRouting:
      // reroute "old" transfer requests
      generate_and_store_transfer_results(data_request_type::kPartialOld);
      // route "new" transfer requests
      generate_and_store_transfer_results(data_request_type::kPartialUpdate);
      break;
  }

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

void storage_updater::generate_and_store_transfer_results(
    data_request_type const request_type) {
  p::routing_graph rg;
  ps::read_routing_graph(rg, storage_.ppr_rg_path_);

  auto treqs =
      to_transfer_requests(storage_.get_transfer_requests_keys(request_type),
                           storage_.get_all_matchings());

  storage_.add_new_transfer_results(std::move(route_multiple_requests(
      treqs, rg, storage_.profile_key_to_search_profile_)));
}

}  // namespace transfers
