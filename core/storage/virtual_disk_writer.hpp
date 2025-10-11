// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "concepts/bitflag.hpp"
#include "storage/disk_info.hpp"

#include <bit>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <vector>

namespace Perimortem::Storage {

class VirtualDiskWriter {
public:
  auto create(DiskType format,
              CompressionLevels compression = CompressionLevels::Default)
      -> void;

  // Populating Disk
  auto write_resource(const std::string_view &path, const Byte *data,
                      const SizeBlock size, StorageOptionsFlags flags) -> void;

  // Write the actual virtual disk
  auto write_disk(const std::filesystem::path &path) -> bool;
  inline auto file_count() -> uint32_t { return files_stored; }

private:
  auto compress(Bytes &source) -> bool;
  auto compress(Bytes &output, ByteView source) -> bool;

  DiskType format = DiskType::Standard;
  CompressionLevels compression = CompressionLevels::Default;
  Byte format_block[sizeof(autogenetic_header) + sizeof(DiskType)] = {0};
  std::vector<Bytes> data_blocks; // File table, File data
  uint32_t files_stored = 0;
};

} // namespace Perimortem::Storage
