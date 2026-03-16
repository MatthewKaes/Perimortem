// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "core/memory/managed/bytes.hpp"
#include "core/memory/managed/vector.hpp"
#include "core/memory/standard_types.hpp"
#include "core/memory/view/bitflag.hpp"
#include "storage/disk_info.hpp"

#include <filesystem>

namespace Perimortem::Storage {

class VirtualDiskWriter {
 public:
  auto create(DiskType format,
              CompressionLevels compression = CompressionLevels::Default)
      -> void;

  // Populating Disk
  auto write_resource(const Memory::View::Bytes path,
                      const Memory::View::Bytes data,
                      StorageFlags flags) -> void;

  // Write the actual virtual disk
  auto write_disk(const std::filesystem::path& path) -> bool;
  inline auto file_count() -> Count { return files_stored; }

 private:
  auto compress(Memory::Managed::Bytes& output, Memory::View::Bytes source)
      -> bool;

  Memory::Arena disk_arena;
  DiskType format = DiskType::Standard;
  CompressionLevels compression = CompressionLevels::Default;
  Memory::Managed::Vector<Memory::Managed::Bytes>
      data_blocks;  // File table, File data
  Count files_stored = 0;
};

}  // namespace Perimortem::Storage
