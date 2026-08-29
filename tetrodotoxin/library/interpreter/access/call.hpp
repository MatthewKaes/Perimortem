// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/access/call.hpp"
#include "ttx/bootstrap/concept/abstract.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter::Access {

// Call reads one callable name and its complete argument Pack. The retained
// Call selects its Callable during linking and keeps the same producers for
// fitting and lowering.
class Call {
 public:
  Call() = delete;

  static auto parse(
      const Ttx::Concept::Abstract& context,
      Ttx::Lexical::Cursor& cursor,
      Language::Model::Pack& receiver)
      -> Perimortem::Core::Option<Language::Expression&>;
};

}  // namespace Tetrodotoxin::Library::Interpreter::Access
