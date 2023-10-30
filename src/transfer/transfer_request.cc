#include "transfers/transfer/transfer_request.h"

#include "transfers/types.h"

#include "fmt/core.h"

#include "utl/progress_tracker.h"
#include "utl/verify.h"

namespace transfers {

std::vector<transfer_request> to_transfer_requests(
    std::vector<transfer_request_by_keys> const& treqs_k,
    hash_map<nlocation_key_t, platform> const& matches) {
  auto treqs = std::vector<transfer_request>{};

  for (auto const& treq_k : treqs_k) {
    auto treq = transfer_request{};

    treq.from_nloc_key_ = treq_k.from_nloc_key_;
    treq.profile_ = treq_k.profile_;

    // extract from_pf
    treq.transfer_start_ = matches.at(treq_k.from_nloc_key_);

    // extract to_pfs
    for (auto to_nloc_key : treq_k.to_nloc_keys_) {
      treq.transfer_targets_.emplace_back(matches.at(to_nloc_key));
      treq.to_nloc_keys_.emplace_back(to_nloc_key);
    }

    treqs.emplace_back(treq);
  }

  return treqs;
}

std::vector<transfer_request_by_keys>
generate_all_pair_transfer_requests_by_keys(
    transfer_request_generation_data const& data,
    transfer_request_options const& opts) {
  auto result = std::vector<transfer_request_by_keys>{};
  auto const profiles = data.profile_key_to_search_profile_;

  auto const all_pairs_trs =
      [&profiles](
          transfer_request_generation_data::matched_nigiri_location_data const&
              from,
          transfer_request_generation_data::matched_nigiri_location_data const&
              to,
          profile_key_t const& prf_key) {
        auto from_to_trs = std::vector<transfer_request_by_keys>{};
        auto const& profile = profiles.at(prf_key);
        auto prf_dist = profile.walking_speed_ * profile.duration_limit_;

        if (from.matched_pfs_idx_.size() == 0 ||
            to.matched_pfs_idx_.size() == 0) {
          return from_to_trs;
        }

        for (auto i = std::size_t{0}; i < from.matched_pfs_idx_.size(); ++i) {

          auto from_pf = from.matched_pfs_idx_.get_platform(i);
          auto target_ids = to.matched_pfs_idx_.get_other_platforms_in_radius(
              from_pf, prf_dist);
          auto from_nloc_key = from.nloc_keys_[i];

          if (target_ids.empty()) {
            continue;
          }

          auto tmp = transfer_request_by_keys{};
          auto to_nloc_keys = vector<nlocation_key_t>{};

          for (auto t_id : target_ids) {
            to_nloc_keys.emplace_back(to.nloc_keys_[t_id]);
          }

          tmp.from_nloc_key_ = from_nloc_key;
          tmp.to_nloc_keys_ = to_nloc_keys;
          tmp.profile_ = prf_key;

          from_to_trs.emplace_back(tmp);
        }

        return from_to_trs;
      };

  auto progress_tracker = utl::get_active_progress_tracker();

  // new possible transfers: 1 -> 2, 2 -> 1, 2 -> 2
  for (auto const& [prf_key, prf_info] : profiles) {
    if (opts.old_to_old_) {
      auto trs11 = all_pairs_trs(data.old_, data.old_, prf_key);
      result.insert(result.end(), trs11.begin(), trs11.end());
    }

    progress_tracker->increment();

    if (!data.update_.set_matched_pfs_idx_) {
      progress_tracker->increment(3);
      continue;
    }

    // new transfers from old to update (1 -> 2)
    auto trs12 = all_pairs_trs(data.old_, data.update_, prf_key);
    progress_tracker->increment();
    // new transfers from update to old (2 -> 1)
    auto trs21 = all_pairs_trs(data.update_, data.old_, prf_key);
    progress_tracker->increment();
    // new transfers from update to update (2 -> 2)
    auto trs22 = all_pairs_trs(data.update_, data.update_, prf_key);
    progress_tracker->increment();

    result.insert(result.end(), trs12.begin(), trs12.end());
    result.insert(result.end(), trs21.begin(), trs21.end());
    result.insert(result.end(), trs22.begin(), trs22.end());
  }

  return result;
}

transfer_request_by_keys merge(transfer_request_by_keys const& a,
                               transfer_request_by_keys const& b) {
  auto merged = transfer_request_by_keys{};
  auto added_to_nlocs = set<nlocation_key_t>{};

  utl::verify(
      a.from_nloc_key_ == b.from_nloc_key_,
      "Cannot merge two transfer requests from different nigiri locations.");
  utl::verify(a.profile_ == b.profile_,
              "Cannot merge two transfer requests with different profiles");

  merged.from_nloc_key_ = a.from_nloc_key_;
  merged.profile_ = a.profile_;

  merged.to_nloc_keys_ = a.to_nloc_keys_;

  // build added_keys set
  for (auto const& nloc_key : merged.to_nloc_keys_) {
    added_to_nlocs.insert(nloc_key);
  }

  // insert new and unique nloc/pf keys
  for (auto nloc_key : b.to_nloc_keys_) {
    if (added_to_nlocs.count(nloc_key) == 1) {
      continue;
    }

    merged.to_nloc_keys_.emplace_back(nloc_key);
    added_to_nlocs.insert(nloc_key);
  }

  return merged;
}

string_t to_key(transfer_request_by_keys const& treq_k) {
  return {fmt::format("{}{}", treq_k.from_nloc_key_, treq_k.profile_)};
}

string_t to_key(transfer_request const& treq) {
  return {fmt::format("{}{}", treq.from_nloc_key_, treq.profile_)};
}

std::ostream& operator<<(std::ostream& out, transfer_request const& treq) {
  auto treq_repr = fmt::format("[transfer request] {} has {} locations.",
                               to_key(treq), treq.to_nloc_keys_.size());
  return out << treq_repr;
}

std::ostream& operator<<(std::ostream& out,
                         transfer_request_by_keys const& treq_k) {
  auto treq_k_repr = fmt::format("[transfer request keys] {} has {} locations.",
                                 to_key(treq_k), treq_k.to_nloc_keys_.size());
  return out << treq_k_repr;
}

}  // namespace transfers
