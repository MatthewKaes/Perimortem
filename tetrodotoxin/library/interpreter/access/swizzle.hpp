// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/access/swizzle.hpp"
#include "ttx/bootstrap/concept/abstract.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter::Access {

// Swizzle reads an ordered list of authored names from one Pack receiver. The
// retained Swizzle keeps those names and resolves their actual producers only
// after the receiver Layout is complete.
class Swizzle {
 public:
  Swizzle() = delete;

  static auto parse(
      const Ttx::Concept::Abstract& context,
      Ttx::Lexical::Cursor& cursor,
      Language::Model::Pack& receiver,
      Ttx::Lexical::Span receiver_span)
      -> Perimortem::Core::Option<Language::Expression&>;
};

}  // namespace Tetrodotoxin::Library::Interpreter::Access
