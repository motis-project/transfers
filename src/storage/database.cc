#include "transfers/storage/database.h"

#include <string_view>

#include "cista/hashing.h"
#include "cista/serialization.h"

#include "utl/enumerate.h"

namespace fs = std::filesystem;

namespace transfers {

constexpr auto const kProfilesDB = "profiles";
constexpr auto const kPlatformsDB = "platforms";
constexpr auto const kMatchingsDB = "matchings";
constexpr auto const kTransReqsDB = "transreqs";
constexpr auto const kTransfersDB = "transfers";

inline std::string_view view(cista::byte_buf const& b) {
  return std::string_view{reinterpret_cast<char const*>(b.data()), b.size()};
}

database::database(fs::path const& db_file_path,
                   std::size_t const db_max_size) {
  env_.set_maxdbs(5);
  env_.set_mapsize(db_max_size);
  auto flags = lmdb::env_open_flags::NOSUBDIR | lmdb::env_open_flags::NOSYNC;
  env_.open(db_file_path.c_str(), flags);
  init();
}

void database::init() {
  // create database
  auto txn = lmdb::txn{env_};
  auto profiles_db = profiles_dbi(txn, lmdb::dbi_flags::CREATE);
  platforms_dbi(txn, lmdb::dbi_flags::CREATE);
  matchings_dbi(txn, lmdb::dbi_flags::CREATE);
  transreqs_dbi(txn, lmdb::dbi_flags::CREATE);
  transfers_dbi(txn, lmdb::dbi_flags::CREATE);

  // find highes profiles id in db
  auto cur = lmdb::cursor{txn, profiles_db};
  auto entry = cur.get(lmdb::cursor_op::LAST);
  highest_profile_id_ = profile_key_t{0};
  if (entry.has_value()) {
    highest_profile_id_ =
        cista::copy_from_potentially_unaligned<profile_key_t>(entry->second);
  }

  cur.reset();
  txn.commit();
}

void database::put_profiles(std::vector<string_t> const& prf_names) {
  auto added_indices = std::vector<std::size_t>{};

  auto txn = lmdb::txn{env_};
  auto profiles_db = profiles_dbi(txn);

  for (auto [idx, name] : utl::enumerate(prf_names)) {
    if (auto const r = txn.get(profiles_db, name); r.has_value()) {
      continue;  // profile already in db
    }
    ++highest_profile_id_;
    auto const serialized_key = cista::serialize(highest_profile_id_);
    txn.put(profiles_db, name, view(serialized_key));
    added_indices.emplace_back(idx);
  }

  txn.commit();
}

hash_map<string_t, profile_key_t> database::get_profile_keys() {
  auto keys_with_name = hash_map<string_t, profile_key_t>{};

  auto txn = lmdb::txn{env_, lmdb::txn_flags::RDONLY};
  auto profiles_db = profiles_dbi(txn);
  auto cur = lmdb::cursor{txn, profiles_db};

  for (auto entry = cur.get(lmdb::cursor_op::FIRST); entry.has_value();
       entry = cur.get(lmdb::cursor_op::NEXT)) {
    // Here it is known that the entry has a value. Therefore,
    // kDefaultStringViewPair is never used.
    auto const [name, key] = entry.value();
    keys_with_name.emplace(
        string_t{name},
        cista::copy_from_potentially_unaligned<profile_key_t>(key));
  }

  cur.reset();
  return keys_with_name;
}

hash_map<profile_key_t, string_t> database::get_profile_key_to_name() {
  auto keys_with_name = hash_map<profile_key_t, string_t>{};

  auto txn = lmdb::txn{env_, lmdb::txn_flags::RDONLY};
  auto profiles_db = profiles_dbi(txn);
  auto cur = lmdb::cursor{txn, profiles_db};

  for (auto entry = cur.get(lmdb::cursor_op::FIRST); entry.has_value();
       entry = cur.get(lmdb::cursor_op::NEXT)) {
    // Here it is known that the entry has a value. Therefore,
    // kDefaultStringViewPair is never used.
    auto const [name, key] = entry.value();
    keys_with_name.emplace(
        cista::copy_from_potentially_unaligned<profile_key_t>(key),
        string_t{name});
  }

  cur.reset();
  return keys_with_name;
}

std::vector<std::size_t> database::put_platforms(std::vector<platform>& pfs) {
  auto added_indices = std::vector<std::size_t>{};

  auto txn = lmdb::txn{env_};
  auto platforms_db = platforms_dbi(txn);

  for (auto [idx, pf] : utl::enumerate(pfs)) {
    auto const osm_key = pf.key();
    if (auto const r = txn.get(platforms_db, osm_key); r.has_value()) {
      continue;  // platform already in db
    }

    auto const serialized_pf = cista::serialize(pf);
    txn.put(platforms_db, osm_key, view(serialized_pf));
    added_indices.emplace_back(idx);
  }

  txn.commit();
  return added_indices;
}

std::vector<platform> database::get_platforms() {
  auto pfs = std::vector<platform>{};

  auto txn = lmdb::txn{env_, lmdb::txn_flags::RDONLY};
  auto platforms_db = platforms_dbi(txn);
  auto cur = lmdb::cursor{txn, platforms_db};

  for (auto entry = cur.get(lmdb::cursor_op::FIRST); entry.has_value();
       entry = cur.get(lmdb::cursor_op::NEXT)) {
    // Here it is known that the entry has a value. Therefore,
    // kDefaultStringViewPair is never used.
    auto const [osm_key, pf_serialized] = entry.value();
    pfs.emplace_back(
        cista::copy_from_potentially_unaligned<platform>(pf_serialized));
    entry = cur.get(lmdb::cursor_op::NEXT);
  }

  cur.reset();
  return pfs;
}

std::optional<platform> database::get_platform(std::string const& osm_key) {
  auto txn = lmdb::txn{env_, lmdb::txn_flags::RDONLY};
  auto platforms_db = platforms_dbi(txn);

  auto entry = txn.get(platforms_db, osm_key);

  if (entry.has_value()) {
    return cista::copy_from_potentially_unaligned<platform>(entry.value());
  }

  return {};
}

std::vector<size_t> database::put_matching_results(
    std::vector<matching_result> const& mrs) {
  auto added_indices = std::vector<std::size_t>{};

  auto txn = lmdb::txn{env_};
  auto matchings_db = matchings_dbi(txn);
  auto platforms_db = platforms_dbi(txn);

  for (auto const& [idx, mr] : utl::enumerate(mrs)) {
    auto const loc_key = mr.loc_.key();
    auto const osm_key = mr.pf_.key();

    if (auto const r = txn.get(matchings_db, loc_key); r.has_value()) {
      continue;  // nloc already matched in db
    }
    if (auto const r = txn.get(platforms_db, osm_key); !r.has_value()) {
      continue;  // osm platform not in platform_db
    }

    txn.put(matchings_db, loc_key, osm_key);
    added_indices.emplace_back(idx);
  }

  txn.commit();
  return added_indices;
}

std::vector<std::pair<location, std::string>> database::get_matchings() {
  auto matchings = std::vector<std::pair<location, std::string>>{};

  auto txn = lmdb::txn{env_, lmdb::txn_flags::RDONLY};
  auto matchings_db = matchings_dbi(txn);
  auto cur = lmdb::cursor{txn, matchings_db};

  for (auto entry = cur.get(lmdb::cursor_op::FIRST); entry.has_value();
       entry = cur.get(lmdb::cursor_op::NEXT)) {
    // Here it is known that the entry has a value. Therefore,
    // kDefaultStringViewPair is never used.
    auto const [loc_key, osm_key] = entry.value();
    // TODO (C) vrfy unaligned..
    matchings.emplace_back(
        location(
            cista::copy_from_potentially_unaligned<location_key_t>(loc_key)),
        std::string{osm_key});
  }

  cur.reset();
  return matchings;
}

hash_map<location_key_t, platform> database::get_loc_to_pf_matchings() {
  auto loc_pf_matchings = hash_map<location_key_t, platform>{};

  for (auto& [location, osm_key] : get_matchings()) {
    auto const pf = get_platform(osm_key);

    if (pf.has_value()) {
      loc_pf_matchings.emplace(location.key(), pf.value());
    }
  }

  return loc_pf_matchings;
}

std::vector<std::size_t> database::put_transfer_requests_by_keys(
    std::vector<transfer_request_by_keys> const& treqs_k) {
  auto added_indices = std::vector<std::size_t>{};

  auto txn = lmdb::txn{env_};
  auto transreqs_db = transreqs_dbi(txn);

  for (auto const& [idx, treq_k] : utl::enumerate(treqs_k)) {
    auto const treq_key = treq_k.key();

    if (auto const r = txn.get(transreqs_db, treq_key); r.has_value()) {
      continue;  // transfer request already in db
    }

    auto const serialized_treq = cista::serialize(treq_k);
    txn.put(transreqs_db, treq_key, view(serialized_treq));
    added_indices.emplace_back(idx);
  }

  txn.commit();
  return added_indices;
}

/**
 * merge and update: transfer_request_keys in db
 */
std::vector<std::size_t> database::update_transfer_requests_by_keys(
    std::vector<transfer_request_by_keys> const& treqs_k) {
  auto updated_indices = std::vector<std::size_t>{};
  auto treq_chashing = cista::hashing<transfer_request_by_keys>{};

  auto txn = lmdb::txn{env_};
  auto transreqs_db = transreqs_dbi(txn);

  for (auto const& [idx, treq] : utl::enumerate(treqs_k)) {
    auto treq_key = treq.key();

    if (auto const r = txn.get(transreqs_db, treq_key); !r.has_value()) {
      continue;  // transfer request not in db
    }

    // Here it is known that the entry has a value. Therefore,
    // kDefaultStringViewPair is never used.
    auto const trans_req_by_key_serialized =
        txn.get(transreqs_db, treq_key).value();

    auto treq_from_db =
        cista::copy_from_potentially_unaligned<transfer_request_by_keys>(
            trans_req_by_key_serialized);
    auto merged = merge(treq_from_db, treq);

    // update entry only in case of changes
    if (treq_chashing(treq_from_db) == treq_chashing(merged)) {
      continue;
    }

    auto const serialized_treq = cista::serialize(merged);
    if (txn.del(transreqs_db, treq_key)) {
      txn.put(transreqs_db, treq_key, view(serialized_treq));
    }

    updated_indices.emplace_back(idx);
  }

  txn.commit();
  return updated_indices;
}

std::vector<transfer_request_by_keys> database::get_transfer_requests_by_keys(
    set<profile_key_t> const& ppr_profiles) {
  auto treqs_k = std::vector<transfer_request_by_keys>{};

  auto txn = lmdb::txn{env_, lmdb::txn_flags::RDONLY};
  auto transreqs_db = transreqs_dbi(txn);
  auto cur = lmdb::cursor{txn, transreqs_db};

  for (auto entry = cur.get(lmdb::cursor_op::FIRST); entry.has_value();
       entry = cur.get(lmdb::cursor_op::NEXT)) {
    // Here it is known that the entry has a value. Therefore,
    // kDefaultStringViewPair is never used.
    auto const [key, trans_req_by_keys] = entry.value();

    auto const db_treq_k =
        cista::copy_from_potentially_unaligned<transfer_request_by_keys>(
            trans_req_by_keys);

    // extract only transfer_requests with requested profiles
    if (ppr_profiles.count(db_treq_k.profile_) == 1) {
      treqs_k.emplace_back(db_treq_k);
    }
  }

  cur.reset();
  return treqs_k;
}

std::vector<std::size_t> database::put_transfer_results(
    std::vector<transfer_result> const& trs) {
  auto added_indices = std::vector<std::size_t>{};

  auto txn = lmdb::txn{env_};
  auto transfers_db = transfers_dbi(txn);

  for (auto const& [idx, tres] : utl::enumerate(trs)) {
    auto const tr_key = tres.key();

    if (auto const r = txn.get(transfers_db, tr_key); r.has_value()) {
      continue;  // transfer already in db
    }

    auto const serialized_tr = cista::serialize(tres);
    txn.put(transfers_db, tr_key, view(serialized_tr));
    added_indices.emplace_back(idx);
  }

  txn.commit();
  return added_indices;
}

/**
 * merge and update: transfer_results in db
 */
std::vector<std::size_t> database::update_transfer_results(
    std::vector<transfer_result> const& trs) {
  auto updated_indices = std::vector<std::size_t>{};
  auto tres_chashing = cista::hashing<transfer_result>{};

  auto txn = lmdb::txn{env_};
  auto transfers_db = transfers_dbi(txn);

  for (auto const& [idx, tres] : utl::enumerate(trs)) {
    auto const tres_key = tres.key();

    if (auto const r = txn.get(transfers_db, tres_key); !r.has_value()) {
      continue;  // transfer not in db
    }

    // Here it is known that the entry has a value. Therefore,
    // kDefaultStringViewPair is never used.
    auto const trans_res_serialized = txn.get(transfers_db, tres_key).value();
    auto tres_from_db = cista::copy_from_potentially_unaligned<transfer_result>(
        trans_res_serialized);
    auto merged = merge(tres_from_db, tres);

    // update entry only in case of changes
    if (tres_chashing(tres_from_db) == tres_chashing(merged)) {
      continue;
    }

    auto const serialized_tr = cista::serialize(merged);
    if (txn.del(transfers_db, tres_key)) {
      txn.put(transfers_db, tres_key, view(serialized_tr));
    }

    updated_indices.emplace_back(idx);
  }

  txn.commit();
  return updated_indices;
}

std::vector<transfer_result> database::get_transfer_results(
    set<profile_key_t> const& ppr_profile_names) {
  auto trs = std::vector<transfer_result>{};

  auto txn = lmdb::txn{env_, lmdb::txn_flags::RDONLY};
  auto transfers_db = transfers_dbi(txn);
  auto cur = lmdb::cursor(txn, transfers_db);

  for (auto entry = cur.get(lmdb::cursor_op::FIRST); entry.has_value();
       entry = cur.get(lmdb::cursor_op::NEXT)) {
    // Here it is known that the entry has a value. Therefore,
    // kDefaultStringViewPair is never used.
    auto const [key, trans_res] = entry.value();

    auto const db_tr =
        cista::copy_from_potentially_unaligned<transfer_result>(trans_res);

    // extract only transfer_results with requested profiles
    if (ppr_profile_names.count(db_tr.profile_) == 1) {
      trs.emplace_back(db_tr);
    }
  }

  cur.reset();
  return trs;
}

lmdb::txn::dbi database::profiles_dbi(lmdb::txn& txn, lmdb::dbi_flags flags) {
  return txn.dbi_open(kProfilesDB, flags);
}

lmdb::txn::dbi database::platforms_dbi(lmdb::txn& txn,
                                       lmdb::dbi_flags const flags) {
  return txn.dbi_open(kPlatformsDB, flags);
}

lmdb::txn::dbi database::matchings_dbi(lmdb::txn& txn,
                                       lmdb::dbi_flags const flags) {
  return txn.dbi_open(kMatchingsDB, flags);
}

lmdb::txn::dbi database::transreqs_dbi(lmdb::txn& txn,
                                       lmdb::dbi_flags const flags) {
  return txn.dbi_open(kTransReqsDB, flags);
}

lmdb::txn::dbi database::transfers_dbi(lmdb::txn& txn,
                                       lmdb::dbi_flags const flags) {
  return txn.dbi_open(kTransfersDB, flags);
}

}  // namespace transfers
