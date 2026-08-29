// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "ttx/bootstrap/concept/abstract.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter::Access {

// Postfix owns the compact grammar for Type access, propagation, and Option
// unwrap. Their semantic objects keep only the receiver and authored evidence
// needed for later linking.
class Postfix {
 public:
  Postfix() = delete;

  static auto parse_type(
      const Ttx::Concept::Abstract& context,
      Ttx::Lexical::Cursor& cursor,
      Language::Model::Pack& receiver)
      -> Perimortem::Core::Option<Language::Expression&>;

  static auto parse_propagate(
      const Ttx::Concept::Abstract& context,
      Ttx::Lexical::Cursor& cursor,
      Language::Model::Pack& receiver)
      -> Perimortem::Core::Option<Language::Expression&>;

  static auto parse_unwrap(
      const Ttx::Concept::Abstract& context,
      Ttx::Lexical::Cursor& cursor,
      Language::Model::Pack& receiver)
      -> Perimortem::Core::Option<Language::Expression&>;
};

}  // namespace Tetrodotoxin::Library::Interpreter::Access
