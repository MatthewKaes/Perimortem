// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/dynamic/record.hpp"
#include "perimortem/storage/archive/info.hpp"
#include "perimortem/storage/archive/reader.hpp"
#include "perimortem/storage/path.hpp"
#include "perimortem/memory/view/bitflag.hpp"

namespace Perimortem::Storage {

// TODO: Should add thread safty at some point but for now just assume main
// thread access.
class Storage {
 public:
  // Resources are used to manage all data that is read and cached into the
  // system. Resrouces can reference files on disk or files in memory.
  struct Resource {
    using Time = Count;

    auto read_content() const -> const Memory::View::Bytes;
    auto write_content(const Memory::Managed::Bytes&& data) -> void;

    inline auto modified_time() const -> Time { return time; };
    inline auto in_memory() const -> bool { return loaded; }
    inline auto is_virtual() const -> bool {
      return flags == Storage::StorageOptions::Virtualized ||
             flags == Storage::StorageOptions::Streamed;
    }

    inline Resource(const Resource&& rhs)
        : content(std::move(rhs.content)),
          source(rhs.source),
          time(rhs.time),
          dirty(rhs.dirty),
          flags(rhs.flags) {}
    inline Resource& operator=(const Resource&& rhs) {
      content = std::move(rhs.content);
      source_file = rhs.source_file;
      dirty = rhs.dirty;
      time = rhs.time;
      flags = rhs.flags;

      return *this;
    }

    inline ~Resource() {}
    Memory::Dynamic::Record content;
    Time time;
    Storage::StorageFlags flags;
  };

  Manager(const std::filesystem::path& data_root);
  ~Manager();

  // Fetching Resources
  auto lookup(const Path& p) -> Resource*;
  auto exists(const Path& p) -> bool;
  auto copy(const Path& src, const Path& dest) -> bool;
  auto move(const Path& src, const Path& dest) -> bool;
  // Deletes a file if it exists in the directory.
  auto remove(const Path& p) -> bool;
  // Creates a file to be managed with the disk.
  auto create(const Path& p,
              Memory::View::Bytes&& data = Memory::View::Bytes(),
              Storage::StorageFlags options =
                  Storage::StorageOptions::Virtualized) -> Resource*;
  // Imports a file and stores it in the requested virtual path.
  // If the file isn't virtual it will be copied on disk. If the file is already
  // imported use other methods to manipulate the file.
  auto import(const Path& p,
              std::filesystem::path src,
              Storage::StorageFlags options = Storage::StorageOptions::None)
      -> Resource*;

  // Changes the properties of a file. This can cause a disk load in the event
  // of preloading or virtualizing a file. Devirtualization does not result in
  // any loads.
  auto change_file_flags(const Path& p, Storage::StorageFlags options) -> bool;

  // Configures a specific sector to use a different disk type for it's
  // autogenetic export.
  auto set_sector_type(Path::Sector sector, Storage::Type type) -> void;

  // Managing data
  auto flush_changes() -> void;

 private:
  struct SectorData {
    auto create(const std::filesystem::path& autogenetic) -> bool;

    std::unique_ptr<Storage::Reader> reader;
    Storage::Type type;
  };

  auto load_file(const Path& p, Resource& res) -> void;
  auto flush_file(const Path& p) -> void;

  std::filesystem::path data_root;
  SectorData sectors[Path::sector_count];
  // std::unordered_map<Path, std::unique_ptr<Resource>> directory;
};

using Resource = Manager::Resource;

}  // namespace Perimortem::Storage
