#pragma once

#include "transfers/storage/storage.h"

#include "nigiri/timetable.h"

namespace transfers {

enum class first_update { kNoUpdate, kProfiles, kTimetable, kOSM };
enum class routing_type { kNoRouting, kPartialRouting, kFullRouting };

struct storage_updater {
  explicit storage_updater(storage storage) : storage_(std::move(storage)) {}

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

  storage storage_;
};

}  // namespace transfers
