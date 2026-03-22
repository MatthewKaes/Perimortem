// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/table.hpp"
#include "perimortem/memory/managed/vector.hpp"
#include "perimortem/memory/view/bitflag.hpp"
#include "perimortem/storage/archive/info.hpp"
#include "perimortem/storage/path.hpp"

namespace Perimortem::Storage::Archive {

class Reader {
 public:
  struct Block {
    Block(Memory::Arena& disk_arena) : data(disk_arena) {}
    Memory::Managed::Bytes data;
    Count location;
  };
  
  Reader()
      : disk_arena(),
        blocks(disk_arena),
        files(disk_arena),
        stream_index(disk_arena) {};

  // Populating Disk
  static auto mount_folder(Path folder) -> Reader;
  static auto mount_archive(Path archive) -> Reader;

  inline auto get_format() const -> Type { return format; }
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

  Memory::Arena disk_arena;
  Type format;
  std::filesystem::path disk_path;
  static auto decompress_block(Memory::Managed::Bytes& data) -> void;

  // Process functions
  auto process_split_table() -> void;
  auto process_inline_table() -> void;

  Memory::Managed::Vector<Block> blocks;
  Memory::Managed::Vector<FileData> files;
  Memory::Managed::Table<Long> stream_index;
};

}  // namespace Perimortem::Storage::Archive
