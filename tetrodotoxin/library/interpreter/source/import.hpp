// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/import.hpp"
#include "ttx/concept/documentation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter::Source {

// Import reads one complete `using` statement. The retained Import receives
// its exact route and source range while recovery remains local to this
// grammar entry.
class Import {
 public:
  Import() = delete;

  static auto parse(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation)
      -> Perimortem::Core::Option<Language::Import>;
};

}  // namespace Tetrodotoxin::Library::Interpreter::Source
