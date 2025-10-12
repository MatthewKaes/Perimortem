// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "resource/path.hpp"

#include "storage/disk_info.hpp"
#include "storage/virtual_disk_reader.hpp"

#include <filesystem>
#include <unordered_map>
#include <vector>

namespace Perimortem::Resource {

// TODO: Should add thread safty at some point but for now just assume main
// thread access.
class Manager {
 public:
  // Resources are used to manage all data that is read and cached into the
  // system. Resrouces can reference files on disk or files in memory.
  struct Resource {
    using Time = uint64_t;

    friend Manager;

    auto read_content() const -> const Storage::ByteView;
    auto write_content(const Storage::Bytes&& data) -> void;

    inline auto modified_time() const -> Time { return time; };
    inline auto in_memory() const -> bool { return loaded; }
    inline auto is_virtual() const -> bool {
      return flags == Storage::StorageOptions::Virtualized ||
             flags == Storage::StorageOptions::Streamed;
    }
    inline Resource(const Resource&& rhs)
        : content(std::move(rhs.content)),
          source(rhs.source),
          dirty(rhs.dirty),
          time(rhs.time),
          flags(rhs.flags) {}
    inline Resource& operator=(const Resource&& rhs) {
      content = std::move(rhs.content);
      source = rhs.source;
      dirty = rhs.dirty;
      time = rhs.time;
      flags = rhs.flags;

      return *this;
    }

    inline ~Resource() {}

   protected:
    auto clone() const -> std::unique_ptr<Resource> {
      Resource copy;
      const auto& view = read_content();
      copy.content = Storage::Bytes(view.begin(), view.end());
      copy.dirty = true;
      copy.loaded = loaded;
      copy.time = time;
      copy.flags = flags;

      return std::make_unique<Resource>(std::move(copy));
    };

    inline Resource()
        : content(), source(), dirty(false), loaded(false), time() {}

    Storage::Bytes content;
    Storage::ByteView source;
    Time time;
    bool dirty = false;
    bool loaded = false;
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
              Storage::Bytes&& data = Storage::Bytes(),
              Storage::StorageFlags options =
                  Storage::StorageOptions::Virtualized) -> Resource*;
  // Imports a file and stores it in the requested virtual path.
  // If the file isn't virtual it will be copied on disk. If the file is already
  // imported use other methods to manipulate the file.
  auto import(const Path& p,
              std::filesystem::path src,
              Storage::StorageFlags options = Storage::StorageOptions::None)
      -> Resource*;

  // Set the path to be a real filesystem or a compressed autogenetic
  // filesystem.
  auto config(const Path& p, Storage::StorageFlags options) -> bool;
  auto config(Path::Sector sector, Storage::DiskType type) -> bool;

  // Managing data
  auto flush_changes() -> void;

 private:
  struct SectorData {
    auto create(const std::filesystem::path& autogenetic) -> bool;

    std::unique_ptr<Storage::VirtualDiskReader> reader;
    Storage::DiskType type;
  };

  auto load_file(const Path& p, Resource& res) -> void;
  auto flush_file(const Path& p) -> void;

  std::filesystem::path data_root;
  SectorData sectors[Path::sector_count];
  std::unordered_map<Path, std::unique_ptr<Resource>> directory;
};

using Resource = Manager::Resource;

}  // namespace Perimortem::Resource
