// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

namespace Perimortem::System {

class File {
 public:
  // Creates an empty file in memory.
  File() = default;

  // Creates an a file of the requested size in bytes.
  File(Count size);

  // Creates a file reference.
  File(Core::View::Bytes data);

  auto write(Core::View::Bytes location) const -> Bool;
  static auto read(Core::View::Bytes location) -> File;

  static auto remove(Core::View::Bytes location) -> Bool;
  static auto exists(Core::View::Bytes location) -> Bool;

  constexpr auto is_valid() const -> Bool { return valid; }
  constexpr auto get_size() const -> Count { return data.get_size(); }

  constexpr auto get_view() const -> const Core::View::Bytes {
    return data.get_view();
  }

  auto get_access() -> Core::Access::Bytes { return data.get_access(); }

  static auto get_root_path() -> Core::View::Bytes;
  static auto set_root_path(Core::View::Bytes path) -> Bool;

 private:
  Memory::Dynamic::Bytes data;
  Bool valid = false;
  Bool virtualized = false;
};

}  // namespace Perimortem::System
