// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "concepts/bitflag.hpp"
#include "concepts/standard_types.hpp"
#include "memory/managed/bytes.hpp"
#include "memory/managed/vector.hpp"
#include "storage/disk_info.hpp"

#include <filesystem>

namespace Perimortem::Storage {

class VirtualDiskWriter {
 public:
  auto create(DiskType format,
              CompressionLevels compression = CompressionLevels::Default)
      -> void;

  // Populating Disk
  auto write_resource(const std::string_view& path,
                      const Memory::View::Bytes data,
                      const SizeBlock size,
                      StorageFlags flags) -> void;

  // Write the actual virtual disk
  auto write_disk(const std::filesystem::path& path) -> bool;
  inline auto file_count() -> uint32_t { return files_stored; }

 private:
  auto compress(Memory::Managed::Bytes& source) -> bool;
  auto compress(Memory::Managed::Bytes& output, Memory::View::Bytes source) -> bool;

  DiskType format = DiskType::Standard;
  CompressionLevels compression = CompressionLevels::Default;
  Byte format_block[sizeof(autogenetic_header) + sizeof(DiskType)] = {0};
  Memory::Managed::Vector<Memory::Managed::Bytes> data_blocks;  // File table, File data
  uint32_t files_stored = 0;
};

}  // namespace Perimortem::Storage
