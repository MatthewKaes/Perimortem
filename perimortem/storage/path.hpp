// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/const/standard_types.hpp"
#include "perimortem/memory/view/bytes.hpp"

namespace Perimortem::Storage {

class Path {
 public:
  enum class Sector : Bits_8 {
    User = 0,
    Scripts = 1,
    Resource = 2,

    // Enable bit flags.
    TOTAL_FLAGS,
    None = static_cast<Bits_8>(-1),
  };

  static constexpr Count header_size = "[???]://"_view.get_size();
  static constexpr Count sector_count = (int)Path::Sector::TOTAL_FLAGS;
  static constexpr Memory::View::Bytes sector_headers[sector_count] = {
      "[usr]://"_view, "[ttx]://"_view, "[res]://"_view};
  static constexpr Memory::View::Bytes logical_disks[sector_count] = {
      "[usr]"_view, "[ttx]"_view, "[res]"_view};
  static constexpr Memory::View::Bytes logical_maps[sector_count] = {
      ""_view, "[ttx]/"_view, "[res]/"_view};

  static_assert(sizeof(Bits_64) == header_size,
                "Header should be exactly 8 bytes.");

  Path(Sector sector, const Memory::View::Bytes local_path);
  Path(Memory::View::Bytes raw_path);

  constexpr inline auto get_path() const -> Memory::View::Bytes { return path; }
  constexpr inline auto get_sector() const -> Sector { return sector; }
  constexpr inline auto get_origin() const -> Memory::View::Bytes {
    return logical_maps[static_cast<Bits_8>(sector)];
  };
  constexpr static auto valid(Sector sector) -> bool {
    return static_cast<Bits_8>(sector) > static_cast<Bits_8>(Sector::None) &&
           static_cast<Bits_8>(sector) <
               static_cast<Bits_8>(Sector::TOTAL_FLAGS);
  }

  constexpr inline Path(const Path& rhs) : path(rhs.path), sector(rhs.sector) {}
  constexpr inline auto operator==(const Path& rhs) const -> bool {
    return sector == rhs.sector && path == rhs.path;
  }

 private:
  Path() = delete;

  Memory::View::Bytes path;
  Sector sector = Sector::User;
};

}  // namespace Perimortem::Storage
