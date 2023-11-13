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
  storage_.update_tt(nigiri_dump_path_);
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
      if (storage_.has_transfer_requests_by_keys(
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
  storage_.update_tt(nigiri_dump_path_);
}

void storage_updater::extract_and_store_osm_platforms() {
  progress_tracker_->status("Extract OSM Platforms")
      .out_bounds(0.F, 5.F)
      .in_high(1);
  auto extracted_platforms = extract_platforms_from_osm_file(osm_path_);
  storage_.add_new_platforms(extracted_platforms);
  progress_tracker_->increment();
}

void storage_updater::match_and_store_matches_by_distance() {
  auto const matching_data = storage_.get_matching_data();

  progress_tracker_->status("Match Nigiri Locations and OSM Platforms.")
      .out_bounds(5.F, 15.F)
      .in_high(matching_data.locations_to_match_.ids_.size());

  auto matcher = distance_matcher(
      matching_data, {max_matching_dist_, max_bus_stop_matching_dist_});

  progress_tracker_->status("Save Matchings.");
  auto const matchings = matcher.matching();
  storage_.add_new_matching_results(matchings);
}

void storage_updater::generate_and_store_transfer_requests(
    bool const old_to_old) {
  auto const treq_gen_data = storage_.get_transfer_request_generation_data();

  progress_tracker_->status("Generate Transfer Requests.")
      .out_bounds(15.F, 30.F)
      .in_high(4 * treq_gen_data.profile_key_to_search_profile_.size());
  auto const generated_trans_reqs = generate_all_pair_transfer_requests_by_keys(
      storage_.get_transfer_request_generation_data(),
      {.old_to_old_ = old_to_old});
  storage_.add_new_transfer_requests_by_keys(generated_trans_reqs);
}

void storage_updater::generate_and_store_transfer_results(
    data_request_type const request_type) {
  p::routing_graph rg;
  ps::read_routing_graph(rg, ppr_rg_path_.string());
  rg.prepare_for_routing(
      rg_config_.edge_rtree_size_, rg_config_.area_rtree_size_,
      rg_config_.lock_rtree_ ? ::ppr::rtree_options::LOCK
                             : ::ppr::rtree_options::PREFETCH);

  auto treqs =
      to_transfer_requests(storage_.get_transfer_requests_by_keys(request_type),
                           storage_.get_all_matchings());

  progress_tracker_->status("Generate Transfer Results.")
      .out_bounds(30.F, 90.F)
      .in_high(treqs.size());

  storage_.add_new_transfer_results(route_multiple_requests(
      treqs, rg, storage_.profile_key_to_search_profile_));
}

}  // namespace transfers
