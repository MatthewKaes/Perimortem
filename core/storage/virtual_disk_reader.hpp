// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "concepts/bitflag.hpp"
#include "storage/disk_info.hpp"

#include <filesystem>
#include <memory>

#include "core/memory/managed/bytes.hpp"
#include "core/memory/managed/table.hpp"
#include "core/memory/managed/vector.hpp"

namespace Perimortem::Storage {

class VirtualDiskReader {
 public:
  struct Block {
    Block(Memory::Arena& disk_arena) : data(disk_arena) {}
    Memory::Managed::Bytes data;
    uint64_t location;
  };

  // Populating Disk
  static auto mount_folder(const std::filesystem::path& disk_path)
      -> std::unique_ptr<VirtualDiskReader>;
  static auto mount_disk(const std::filesystem::path& disk_path)
      -> std::unique_ptr<VirtualDiskReader>;

  inline auto get_format() const -> DiskType { return format; }
  inline auto get_blocks() const -> const Memory::View::Vector<Block> {
    return blocks.get_view();
  }
  inline auto get_files() const -> const Memory::View::Vector<FileData> {
    return files.get_view();
  }

  // Read from a stream instance
  auto stream_from_disk(const Memory::View::Bytes path,
                        Memory::Managed::Bytes& data) -> bool;

  // Semi-human readable dump of the disk information
  auto dump_info() const -> std::string;

 private:
  VirtualDiskReader()
      : disk_arena(),
        blocks(disk_arena),
        files(disk_arena),
        stream_index(disk_arena) {};

  Memory::Arena disk_arena;
  DiskType format;
  std::filesystem::path disk_path;
  static auto decompress_block(Memory::Managed::Bytes& data) -> void;

  // Process functions
  auto process_split_table() -> void;
  auto process_inline_table() -> void;

  Memory::Managed::Vector<Block> blocks;
  Memory::Managed::Vector<FileData> files;
  Memory::Managed::Table<int64_t> stream_index;
};

}  // namespace Perimortem::Storage
