// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "core/memory/standard_types.hpp"
#include "core/memory/view/bytes.hpp"

namespace Perimortem::Resource {

class Path {
 public:
  enum class Sector : Bits_8 {
    User = 0,
    Scripts = 1,
    Resource = 2,

    MAX_SECTORS,
    Invalid,
  };

  static constexpr Count header_size = Memory::Const::static_strlen("[???]://");
  static constexpr Count disk_size = Memory::Const::static_strlen("[???]");
  static constexpr Count sector_count = (int)Path::Sector::MAX_SECTORS;
  static constexpr Memory::Const::StackString<header_size>
      sector_headers[sector_count] = {"[usr]://", "[ttx]://", "[res]://"};
  static constexpr Memory::Const::StackString<disk_size>
      logical_disks[sector_count] = {"[usr]", "[ttx]", "[res]"};
  static constexpr Memory::View::Bytes logical_maps[sector_count] = {
      "", "[ttx]/", "[res]/"};

  Path(Sector sector, const Memory::View::Bytes local_path);
  Path(Memory::View::Bytes raw_path);

  constexpr inline auto get_path() const -> Memory::View::Bytes { return path; }
  constexpr inline auto get_sector() const -> Sector { return sector; }
  constexpr inline auto get_origin() const -> Memory::View::Bytes {
    return logical_maps[static_cast<int>(sector)];
  };
  constexpr inline auto valid() const -> bool {
    return static_cast<Bits_8>(sector) <
           static_cast<Bits_8>(Sector::MAX_SECTORS);
  }

  constexpr inline Path(const Path& rhs)
      : path(std::move(rhs.path)), sector(rhs.sector) {}
  constexpr inline auto operator==(const Path& rhs) const -> bool {
    return sector == rhs.sector && path == rhs.path;
  }

 private:
  Path() = delete;

  Memory::View::Bytes path;
  Sector sector = Sector::User;
};

}  // namespace Perimortem::Resource
