// Perimortem Engine
// Copyright © Matt Kaes

#pragma once


#include "perimortem/core/perimortem.hpp"
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
  auto write_disk() -> Core::View::Bytes;
  inline auto get_staged_resources() -> Core::View::Vector<File> {
    return staged_files;
  }

 private:
  auto compress(Memory::Dynamic::Bytes& output, Core::View::Bytes source)
      -> Bool

  Memory::Allocator::Arena disk_arena;
  Type format = Type::Standard;
  CompressionLevel compression = CompressionLevel::Default;
  Memory::Dynamic::Bytes blocks[2];
  Core::View::Vector<File> staged_files;
};

}  // namespace Perimortem::Storage::Archive
