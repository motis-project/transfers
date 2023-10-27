#pragma once

#include <cstddef>
#include <filesystem>

#include "transfers/storage/storage.h"

#include "nigiri/timetable.h"

#include "utl/progress_tracker.h"

namespace transfers {

enum class first_update { kNoUpdate, kProfiles, kTimetable, kOSM };
enum class routing_type { kNoRouting, kPartialRouting, kFullRouting };

struct storage_updater_config {
  // storage
  std::filesystem::path db_file_path_;
  std::size_t db_max_size_;

  // storage updater config
  std::filesystem::path osm_path_;
  std::filesystem::path ppr_rg_path_;
  std::filesystem::path nigiri_dump_path_;

  // matching config
  double max_matching_dist_{400};
  double max_bus_stop_matching_dist_{120};
};

struct storage_updater {
  explicit storage_updater(storage storage) : storage_(std::move(storage)) {}
  explicit storage_updater(::nigiri::timetable& tt,
                           storage_updater_config const& config)
      : storage_(config.db_file_path_, config.db_max_size_, tt),
        osm_path_(config.osm_path_),
        ppr_rg_path_(config.ppr_rg_path_),
        nigiri_dump_path_(config.nigiri_dump_path_),
        max_matching_dist_(config.max_matching_dist_),
        max_bus_stop_matching_dist_(config.max_bus_stop_matching_dist_) {
    storage_.initialize();
  }
  storage_updater(std::filesystem::path const& db_file_path,
                  std::size_t const db_max_size, ::nigiri::timetable& tt)
      : storage_(db_file_path, db_max_size, tt) {
    storage_.initialize();
  }

  // Returns the current timetable version from storage.
  ::nigiri::timetable get_timetable() const { return storage_.tt_; };

  // Performs a complete update of the timetable.
  //
  // To be used when there is no data in the database due to previous
  // application of the transfer calculation.
  void full_update();

  // Performs a partial update of the timetable.
  //
  // To be used when there is data in the database due to previous
  // application of the transfer calculation.
  void partial_update(first_update const, routing_type const);

  storage storage_;

private:
  // Extracts platforms from an OSM file (path given in the storage) and stores
  // them in the database as well as in the storage.
  void extract_and_store_osm_platforms();

  // Matches OSM platforms and nigiri locations (stored in the storage) and
  // stores them in the database and in the storage.
  void match_and_store_matches_by_distance();

  // Generates transfer requests based on the matches (stored in the storage)
  // and stores them in the database and in the storage.
  void generate_and_store_transfer_requests(bool const old_to_old = false);

  // Generates transfer results based on transfer requests (stored in the
  // storage) using ppr and stores them in the database and in the storage.
  //
  // data_request_type: determines the data to be considered.
  void generate_and_store_transfer_results(data_request_type const);

  std::filesystem::path osm_path_;
  std::filesystem::path ppr_rg_path_;
  std::filesystem::path nigiri_dump_path_;

  double max_matching_dist_{400};
  double max_bus_stop_matching_dist_{120};

  utl::progress_tracker_ptr progress_tracker_{
      utl::get_active_progress_tracker()};
};

}  // namespace transfers
