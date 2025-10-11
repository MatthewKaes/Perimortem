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

class VirtualDiskReader {
public:
  struct Block {
    Bytes data;
    int location;
  };

  // Populating Disk
  VirtualDiskReader(const std::filesystem::path &disk_path);

  inline auto get_format() const -> DiskType { return format; }
  inline auto get_blocks() const -> const std::vector<Block> & {
    return blocks;
  }
  inline auto get_files() const -> const std::vector<FileData> & {
    return files;
  }

  // Read from a stream instance
  auto stream_from_disk(const std::string_view &path, Bytes &data) -> bool;

  // Semi-human readable dump of the disk information
  auto dump_info() const -> std::string;

private:
  DiskType format;
  std::vector<Block> blocks;
  std::filesystem::path disk_path;
  auto decompress_block(Bytes &data) -> void;

  // Process functions
  auto process_split_table(Block &table, Block &data) -> void;
  auto process_inline_table(Block &table) -> void;

  std::vector<FileData> files;
  std::unordered_map<std::string_view, int64_t> stream_index;
};

} // namespace Perimortem::Storage
