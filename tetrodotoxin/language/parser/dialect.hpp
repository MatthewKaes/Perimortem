// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Language::Parser {

// Parses a dialect declreation and returns the Dialect name.
class Dialect {
 public:
  static auto parse(Ttx::Lexical::Cursor& cursor)
      -> Perimortem::Core::View::Bytes;
};

}  // namespace Tetrodotoxin::Language::Parser
