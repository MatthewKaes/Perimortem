// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/expressions/initializer.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter::Expressions {

// Initializer reads one explicit `new` expression and hands its target route
// and argument Pack to the retained Initializer. The model keeps evaluation
// order without importing the grammar that produced it.
class Initializer {
 public:
  Initializer() = delete;

  static auto is_next(const Ttx::Lexical::Cursor& cursor) -> Bool;

  static auto parse(
      const Ttx::Concept::Abstract& context,
      Ttx::Lexical::Cursor& cursor)
      -> Perimortem::Core::Option<Language::Expressions::Initializer&>;
};

}  // namespace Tetrodotoxin::Library::Interpreter::Expressions
