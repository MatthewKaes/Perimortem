// Perimortem Engine
// Copyright © Matt Kaes

#include "resource/path.hpp"

using namespace Perimortem::Resource;
using namespace Perimortem::Memory;

Path::Path(Path::Sector sector, const View::Bytes local_path)
      : path(local_path),
        sector(sector) {
  if (path.slice(0, sector_headers[0].size()) == logical_maps[static_cast<Bits_8>(sector)]) {
    path = path.slice(sector_headers[0].size(), -1);
  }

  if (path.get_size() == 0) {
    this->sector = Sector::Invalid;
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