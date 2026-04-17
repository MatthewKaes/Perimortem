// Perimortem Engine
// Copyright © Matt Kaes

#pragma once


#include "perimortem/core/standard_types.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/view/bitflag.hpp"

#include "perimortem/storage/archive/info.hpp"

namespace Perimortem::Storage::Serialize {

class Archive {
 public:
  auto create(Type format,
              CompressionLevel compression = CompressionLevel::Default)
      -> void;

  // Populating Disk
  auto stage_resource(File resource) -> void;

  // Write the actual virtual disk
  auto write_disk() -> Memory::View::Bytes;
  inline auto get_staged_resources() -> Memory::View::Vector<File> {
    return staged_files;
  }

 private:
  auto compress(Memory::Dynamic::Bytes& output, Memory::View::Bytes source)
      -> bool;

  Memory::Allocator::Arena disk_arena;
  Type format = Type::Standard;
  CompressionLevel compression = CompressionLevel::Default;
  Memory::Dynamic::Bytes blocks[2];
  Memory::View::Vector<File> staged_files;
};

}  // namespace Perimortem::Storage::Archive
