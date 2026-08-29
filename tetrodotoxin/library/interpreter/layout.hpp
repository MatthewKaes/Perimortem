// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/model/layout.hpp"
#include "ttx/bootstrap/concept/abstract.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter {

// Layout turns the shared descriptor envelope into Library TypeReferences and
// exact parameter or result edges. Language Parser owns punctuation and names,
// leaving Library responsible only for its receiver and Type semantics.
class Layout {
 public:
  Layout() = delete;

  static auto parse_model(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& host,
      Bool parameters) -> Perimortem::Core::Option<Language::Model::Layout&>;
};

}  // namespace Tetrodotoxin::Library::Interpreter
