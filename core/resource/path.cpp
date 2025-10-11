// Perimortem Engine
// Copyright © Matt Kaes

#include "resource/path.hpp"

using namespace Perimortem::Resource;

Path::Path(Path::Sector sector, const std::string_view &path_)
    : path(logical_maps[(int)sector]), sector(sector) {
  path += path_;
}

Path::Path(const std::string &path_) {
  path = path_;
  sector = Path::Sector::User;
  if (path.size() > header_size && path[0] == '[' && path[4] == ']' &&
      path[5] == ':' && path[6] == '/' && path[7] == '/') {
    for (int i = 0; i < sector_count; i++) {
      if (path_.starts_with(logical_disks[i])) {
        sector = static_cast<Sector>(i);
        path = logical_maps[i];
        path += path_.substr(header_size - 1);
        return;
      }
    }
  }
}
