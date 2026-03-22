// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/storage/path.hpp"
#include "perimortem/memory/dynamic/map.hpp"

using namespace Perimortem::Storage;
using namespace Perimortem::Memory;

static thread_local Dynamic::Map<Path, Bits_64>

Path::Path(Path::Sector sector, const View::Bytes local_path)
      : path(local_path),
        sector(valid(sector) ? sector : Sector::User) {
  if (path.slice(0, sector_headers[0].size()) == logical_maps[static_cast<Bits_8>(sector)]) {
    path = path.slice(sector_headers[0].size(), -1);
  }

  if (path.get_size() == 0) {
    this->sector = Sector::None;
  }
}

Path::Path(View::Bytes raw_path) {
  path = raw_path;
  sector = Path::Sector::User;
  auto header_check = path.slice(0, sector_headers[0].size());
  for (int i = 0; i < sector_count; i++) {
    if (header_check == View::Bytes(sector_headers[i])) {
      sector = static_cast<Sector>(i);
      path = path.slice(sector_headers[0].size(), -1);
      return;
    }
  }
}