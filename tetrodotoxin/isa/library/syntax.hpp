// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa::Library {

// Syntax owns Library-level token recovery and source-span skipping. It does
// not interpret expressions; it only keeps callers aligned on the next
// declaration boundary.
class Syntax {
 public:
  static auto consume_declaration_tail(Ttx::Lexical::Cursor& cursor) -> Bool;
  static auto consume_initializer(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Core::View::Bytes error_message) -> Bool;
};

}  // namespace Tetrodotoxin::Isa::Library
