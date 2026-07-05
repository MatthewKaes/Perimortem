// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa::Library {

// Syntax owns Library-level token recovery and source-span skipping.
// TODO: Replace this once we have the proper ISAs. We are down to one remianing
// function before we can remove Library::Syntax.
class Syntax {
 public:
  static auto consume_declaration_tail(Ttx::Lexical::Cursor& cursor) -> Bool;
};

}  // namespace Tetrodotoxin::Isa::Library
