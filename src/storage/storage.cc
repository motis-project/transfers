#include "transfers/storage/storage.h"

#include "transfers/storage/to_nigiri.h"

#include "nigiri/footpath.h"
#include "nigiri/types.h"

#include "utl/progress_tracker.h"
#include "utl/to_vec.h"

namespace n = ::nigiri;
namespace fs = std::filesystem;

namespace transfers {

void storage::initialize() { load_old_state_from_db(used_profiles_); }

void storage::save_tt(fs::path const& save_to) const { tt_.write(save_to); }

void storage::update_tt(fs::path const& save_to) {
  auto const to_nigiri_transfer_data = get_to_nigiri_data();

  auto const progress_tracker = utl::get_active_progress_tracker();
  progress_tracker->status("Update Timetable")
      .out_bounds(90.F, 100.F)
      .in_high(to_nigiri_transfer_data.transfer_results_.size());

  auto const nigiri_transfers_data =
      build_nigiri_transfers(to_nigiri_transfer_data);
  set_new_timetable_transfers(nigiri_transfers_data);
  save_tt(save_to);
}

matching_data storage::get_matching_data() {
  return {tt_.locations_, old_state_.matches_, *(old_state_.pfs_idx_),
          *(update_state_.pfs_idx_), update_state_.set_pfs_idx_};
}

hash_map<location_key_t, platform> storage::get_all_matchings() {
  auto all_matchings = old_state_.matches_;
  all_matchings.insert(update_state_.matches_.begin(),
                       update_state_.matches_.end());
  return all_matchings;
}

bool storage::has_transfer_requests_by_keys(data_request_type const req_type) {
  return get_transfer_requests_by_keys(req_type).empty();
}

std::vector<transfer_request_by_keys> storage::get_transfer_requests_by_keys(
    data_request_type const req_type) {
  switch (req_type) {
    case data_request_type::kPartialOld:
      return old_state_.transfer_requests_by_keys_;
    case data_request_type::kPartialUpdate:
      return update_state_.transfer_requests_by_keys_;
    case data_request_type::kFull:
      auto full = old_state_.transfer_requests_by_keys_;
      full.insert(full.end(), update_state_.transfer_requests_by_keys_.begin(),
                  update_state_.transfer_requests_by_keys_.end());
      return full;
  }

  return {};
}

transfer_request_generation_data
storage::get_transfer_request_generation_data() {
  return {{*(old_state_.matched_pfs_idx_), old_state_.locs_,
           old_state_.set_matched_pfs_idx_},
          {*(update_state_.matched_pfs_idx_), update_state_.locs_,
           update_state_.set_matched_pfs_idx_},
          profile_key_to_search_profile_};
}

void storage::add_new_profiles(std::vector<string_t> const& profile_names) {
  db_.put_profiles(profile_names);
  profile_name_to_profile_key_ = db_.get_profile_keys();
  profile_key_to_profile_name_ = db_.get_profile_key_to_name();
}

void storage::add_new_platforms(std::vector<platform>& pfs) {
  auto const added_to_db = db_.put_platforms(pfs);
  auto new_pfs = utl::to_vec(
      added_to_db, [&pfs](auto const i) -> platform { return pfs[i]; });

  update_state_.pfs_idx_ = std::make_unique<platform_index>(new_pfs);
  update_state_.set_pfs_idx_ = true;
}

void storage::add_new_matching_results(
    std::vector<matching_result> const& mrs) {
  auto const added_to_db = db_.put_matching_results(mrs);
  auto const new_mrs = utl::to_vec(
      added_to_db, [&mrs](auto const i) -> matching_result { return mrs[i]; });

  auto matched_pfs = std::vector<platform>{};
  for (auto const& mr : new_mrs) {
    update_state_.matches_.emplace(mr.loc_.key(), mr.pf_);
    update_state_.locs_.emplace_back(mr.loc_.key());
    matched_pfs.emplace_back(mr.pf_);
  }

  update_state_.matched_pfs_idx_ =
      std::make_unique<platform_index>(matched_pfs);
  update_state_.set_matched_pfs_idx_ = true;
}

void storage::add_new_transfer_requests_by_keys(
    std::vector<transfer_request_by_keys> const& treqs_k) {
  auto const updated_in_db = db_.update_transfer_requests_by_keys(treqs_k);
  auto const added_to_db = db_.put_transfer_requests_by_keys(treqs_k);
  update_state_.transfer_requests_by_keys_.clear();

  for (auto const i : updated_in_db) {
    update_state_.transfer_requests_by_keys_.emplace_back(treqs_k[i]);
  }

  for (auto const i : added_to_db) {
    update_state_.transfer_requests_by_keys_.emplace_back(treqs_k[i]);
  }
}

void storage::add_new_transfer_results(
    std::vector<transfer_result> const& tres) {
  auto const updated_in_db = db_.update_transfer_results(tres);
  auto const added_to_db = db_.put_transfer_results(tres);
  update_state_.transfer_results_.clear();

  for (auto const i : updated_in_db) {
    update_state_.transfer_results_.emplace_back(tres[i]);
  }

  for (auto const i : added_to_db) {
    update_state_.transfer_results_.emplace_back(tres[i]);
  }
}

void storage::load_old_state_from_db(set<profile_key_t> const& profile_keys) {
  auto old_pfs = db_.get_platforms();
  old_state_.pfs_idx_ = std::make_unique<platform_index>(old_pfs);
  old_state_.set_pfs_idx_ = true;
  old_state_.matches_ = db_.get_loc_to_pf_matchings();
  old_state_.transfer_requests_by_keys_ =
      db_.get_transfer_requests_by_keys(profile_keys);
  old_state_.transfer_results_ = db_.get_transfer_results(profile_keys);

  auto matched_pfs = std::vector<platform>{};
  auto matched_locs = std::vector<location>{};
  for (auto const& [loc_key, pf] : old_state_.matches_) {
    matched_locs.emplace_back(loc_key);
    matched_pfs.emplace_back(pf);
  }
  old_state_.locs_ = matched_locs;
  old_state_.matched_pfs_idx_ = std::make_unique<platform_index>(matched_pfs);
  old_state_.set_matched_pfs_idx_ = true;
}

to_nigiri_data storage::get_to_nigiri_data() {
  auto tress = db_.get_transfer_results(used_profiles_);
  return {tt_.locations_.coordinates_, tt_.profiles_,
          profile_key_to_profile_name_, tress};
}

void storage::reset_timetable_transfers() {
  for (auto prf_idx = n::profile_idx_t{0U}; prf_idx < n::kMaxProfiles;
       ++prf_idx) {
    tt_.locations_.footpaths_out_[prf_idx] =
        n::vecvec<n::location_idx_t, n::footpath>{};
    tt_.locations_.footpaths_in_[prf_idx] =
        n::vecvec<n::location_idx_t, n::footpath>{};
  }
}

void storage::set_new_timetable_transfers(nigiri_transfers const& ntransfers) {
  reset_timetable_transfers();

  for (auto prf_idx = n::profile_idx_t{0U}; prf_idx < n::kMaxProfiles;
       ++prf_idx) {
    for (auto loc_idx = n::location_idx_t{0U};
         loc_idx < tt_.locations_.names_.size(); ++loc_idx) {
      tt_.locations_.footpaths_out_[prf_idx].emplace_back(
          ntransfers.out_[prf_idx][loc_idx]);
    }
  }

  for (auto prf_idx = n::profile_idx_t{0U}; prf_idx < n::kMaxProfiles;
       ++prf_idx) {
    for (auto loc_idx = n::location_idx_t{0U};
         loc_idx < tt_.locations_.names_.size(); ++loc_idx) {
      tt_.locations_.footpaths_in_[prf_idx].emplace_back(
          ntransfers.in_[prf_idx][loc_idx]);
    }
  }
}

}  // namespace transfers
