// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/lexical/cursor.hpp"
#include "ttx/lexical/span.hpp"

namespace Tetrodotoxin::Package::Language {

// Source is the exact authored semantic name to normalized Package path
// binding. It never derives semantic identity from that path.
class Source {
 public:
  constexpr Source(
      Perimortem::Core::View::Bytes local_name,
      Perimortem::Core::View::Bytes source_path)
      : local_name(local_name), source_path(source_path) {}

  // Consumes one complete Source statement and publishes its bounds beside the
  // durable Source. Failure recovers the Cursor and leaves the Span invalid.
  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Lexical::Cursor& cursor,
      Ttx::Lexical::Span& span) -> Perimortem::Core::Option<Source>;

  constexpr auto get_local_name() const -> Perimortem::Core::View::Bytes {
    return local_name;
  }

  constexpr auto get_source_path() const -> Perimortem::Core::View::Bytes {
    return source_path;
  }

 private:
  Perimortem::Core::View::Bytes local_name;
  Perimortem::Core::View::Bytes source_path;
};

}  // namespace Tetrodotoxin::Package::Language
