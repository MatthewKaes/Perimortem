// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/flow/return.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter::Execution {

// Return reads one optional value Pack and retains empty flow explicitly for a
// bare statement. The semantic Return owns that exact Pack through linking.
class Return {
 public:
  Return() = delete;

  static auto parse(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& context)
      -> Perimortem::Core::Option<Language::Flow::Return&>;
};

}  // namespace Tetrodotoxin::Library::Interpreter::Execution
