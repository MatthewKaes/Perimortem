// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/access/address.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter::Access {

// Address reads one postfix member spelling and preserves its authored Anchor.
// Selection remains delayed on the real Address expression until linking has
// the completed receiver context.
class Address {
 public:
  Address() = delete;

  static auto parse(
      const Ttx::Concept::Abstract& context,
      Ttx::Lexical::Cursor& cursor,
      Language::Model::Pack& receiver)
      -> Perimortem::Core::Option<Language::Expression&>;
};

}  // namespace Tetrodotoxin::Library::Interpreter::Access
