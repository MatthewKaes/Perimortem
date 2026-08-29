// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/foreign.hpp"
#include "ttx/bootstrap/concept/documentation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter::Source {

// Foreign reads one external ABI block into the Source owned Foreign context.
// A closing brace commits the complete block so an interrupted declaration
// cannot change an earlier retained ABI surface.
class Foreign {
 public:
  Foreign() = delete;

  static auto is_next(const Ttx::Lexical::Cursor& cursor) -> Bool;

  static auto parse(
      Language::Foreign& host,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation) -> Bool;
};

}  // namespace Tetrodotoxin::Library::Interpreter::Source
