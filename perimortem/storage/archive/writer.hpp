// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/dynamic/record.hpp"
#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"
#include "perimortem/memory/const/standard_types.hpp"
#include "perimortem/memory/view/bitflag.hpp"
#include "perimortem/storage/archive/info.hpp"
#include "perimortem/storage/path.hpp"

namespace Perimortem::Storage::Archive {

class Writer {
 public:
  auto create(Type format,
              CompressionLevels compression = CompressionLevels::Default)
      -> void;

  // Populating Disk
  auto stage_resource(Path path,
                      const Memory::View::Bytes data,
                      StorageFlags flags) -> void;

  // Write the actual virtual disk
  auto write_disk(const std::filesystem::path& path) -> bool;
  inline auto file_count() -> Count { return files_stored; }

 private:
  auto compress(Memory::Managed::Bytes& output, Memory::View::Bytes source)
      -> bool;

  Memory::Arena disk_arena;
  Type format = Type::Standard;
  CompressionLevels compression = CompressionLevels::Default;
  Memory::Managed::Vector<Memory::Managed::Bytes>
      data_blocks;  // File table, File data
  Count files_stored = 0;
};

}  // namespace Perimortem::Storage::Archive
