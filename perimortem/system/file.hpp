// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

namespace Perimortem::System {

// Represents possible bytes
class File {
 public:
  enum class State {
    // The data is assumed to be the same as the source.
    Original,
    // The file in memory has more up to date information.
    Fresh,
    // The source has more up to date information than the file in memory.
    Stale,
    // The file has been mutated but the source has also mutated since last
    // the last read.
    Conflict,
    // The location is empty and writing would create a new file.
    Create,
    // Invalid sync location
    Invalid,
  };

  // Creates an empty file in memory.
  File() = default;

  // Updates the contents of the file in memory to the content in data.
  auto update_contents(Core::View::Bytes content) -> void;
  // Updates the contents of the file in memory to the content in data.
  auto update_contents(Memory::Dynamic::Bytes&& content) -> void;

  auto write(Core::View::Bytes location) const -> Bool;
  auto read(Core::View::Bytes location) -> Bool;

  // Checks the state of the file against a location.
  auto sync_status(Core::View::Bytes location) const -> State;
  // Syncs a memory file with a file on disk.
  // If there is a `State::Conflict` the data on disk will be overwritten by the
  // data in memory only if `force` is set.
  //
  // Return the updated state of the File in memory.
  auto sync(Core::View::Bytes location, Bool force = false) -> State;

  static auto remove(Core::View::Bytes location) -> Bool;
  static auto exists(Core::View::Bytes location) -> Bool;

  constexpr auto is_valid() const -> Bool {
    return read_time != 0 || modified_time != 0;
  }
  constexpr auto get_size() const -> Count { return data.get_size(); }

  constexpr auto get_view() const -> const Core::View::Bytes {
    return data.get_view();
  }

  static auto get_root_path() -> Core::View::Bytes;
  static auto set_root_path(Core::View::Bytes path) -> Bool;

 private:
  auto update_modified_time() -> void;

  Memory::Dynamic::Bytes data;
  Bits_64 read_time = 0;
  Bits_64 modified_time = 0;
};

}  // namespace Perimortem::System
