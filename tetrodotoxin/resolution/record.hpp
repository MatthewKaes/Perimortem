// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/syntax/package.hpp"

namespace Tetrodotoxin::Resolution {

enum class State : Bits_8 {
  Unloaded,
  Parsed,
  Resolving,
  Resolved,
  Failed,
};

class Record {
 public:
  explicit Record(Perimortem::Core::View::Bytes source_path);

  auto load_if_stale() -> Bool;
  auto invalidate_resolution() -> void;
  auto begin_resolution() -> void;
  auto finish_resolution() -> void;
  auto fail() -> void;

  auto get_path() const -> Perimortem::Core::View::Bytes { return source_path; }
  auto get_package() const -> const Syntax::Package& { return package; }
  auto get_state() const -> State { return state; }
  auto get_parse_count() const -> Count { return parse_count; }
  auto get_error_count() const -> Count { return error_count; }
  auto is_loaded() const -> Bool { return state != State::Unloaded; }
  auto is_resolved() const -> Bool { return state == State::Resolved; }

 private:
  auto reload() -> Bool;

  Perimortem::Memory::Dynamic::Bytes source_path;
  Perimortem::System::File source;
  Perimortem::Memory::Allocator::Arena arena;
  Syntax::Package package;
  State state = State::Unloaded;
  Count parse_count = 0;
  Count error_count = 0;
};

}  // namespace Tetrodotoxin::Resolution
