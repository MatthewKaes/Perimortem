// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/type_reference.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter {

// TypeReference reads one delayed authored Type route. The retained value owns
// only source evidence and semantic arguments so later resolution never needs
// to recover parser state from its Anchor.
class TypeReference {
 public:
  TypeReference() = delete;

  static auto parse(
      const Ttx::Concept::Abstract& context,
      Ttx::Lexical::Cursor& cursor)
      -> Perimortem::Core::Option<Language::TypeReference>;

  static auto parse_route(Ttx::Lexical::Cursor& cursor)
      -> Perimortem::Core::Option<Language::TypeReference>;
};

}  // namespace Tetrodotoxin::Library::Interpreter
