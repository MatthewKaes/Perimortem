// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/core/view/bytes.hpp"
#include "perimortem/serialization/archive/info.hpp"

namespace Perimortem::Serialization::Archive {

class Reader {
 public:
  auto Load(Core::View::Bytes archive);

  inline auto get_format() const -> Type { return format; }
  inline auto get_files() const -> const Core::View::Vector<Object> {
    return files.get_view();
  }

  // Semi-human readable dump of the disk information
  auto dump_info() const -> Memory::Dynamic::Bytes;

 private:
  static auto decompress_block(Memory::Managed::Bytes& data) -> void;

  // Process functions
  auto process_split_table() -> void;
  auto process_inline_table() -> void;

  Type format;
  Memory::Dynamic::Vector<Object> files;
  Memory::Dynamic::Bytes blocks[2];
};

}  // namespace Perimortem::Storage::Archive
