// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include <filesystem>
#include <string>

namespace Perimortem::Resource {

// Index headers
constexpr uint8_t header_size = sizeof("[???]://");

class Path {
 public:
  enum class Sector {
    User = 0,
    Scripts = 1,
    Resource = 2,

    MAX_SECTORS,
  };

  static constexpr int sector_count = (int)Path::Sector::MAX_SECTORS;
  static constexpr std::array<std::string_view, Path::sector_count>
      logical_disks = {"[usr]", "[ttx]", "[res]"};
  static constexpr std::array<std::string_view, Path::sector_count>
      logical_maps = {"", "[ttx]/", "[res]/"};

  Path(Sector sector, const std::string_view& local_path);
  Path(const std::string& path);

  inline auto get_path() const -> const std::string& { return path; }
  inline auto get_sector() const -> Sector { return sector; }
  inline auto get_origin() const -> const std::string_view {
    auto header = logical_maps[static_cast<int>(sector)];
    return {path.c_str() + header.size(), path.size() - header.size()};
  };

  inline Path(const Path& rhs)
      : path(std::move(rhs.path)), sector(rhs.sector) {}
  inline auto operator==(const Path& rhs) const -> bool {
    return sector == rhs.sector && path == rhs.path;
  }

 private:
  Path() = delete;

  std::string path;
  Sector sector = Sector::User;
};

}  // namespace Perimortem::Resource

namespace std {
template <>
struct hash<Perimortem::Resource::Path> {
  size_t operator()(const Perimortem::Resource::Path& r) const noexcept {
    return hash<string>()(r.get_path()) ^
           hash<uint32_t>()((uint32_t)r.get_sector());
  }
};
}  // namespace std