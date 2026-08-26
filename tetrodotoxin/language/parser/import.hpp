// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/language/import.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Language::Parser {

// Import parses the common Alias-producing source preamble shared by every
// installed Dialect.
class Import {
 public:
  static auto is_next(const Ttx::Lexical::Cursor& cursor) -> Bool;

  static auto parse(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation)
      -> Perimortem::Core::Option<Language::Import::Description>;
};

}  // namespace Tetrodotoxin::Language::Parser
