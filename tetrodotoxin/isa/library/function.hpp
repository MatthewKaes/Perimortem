// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/utility/range.hpp"

#include "tetrodotoxin/isa/library/scope.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Library {

// Function parses a stable TTX signature and captures the source range of a
// definition body. Body execution waits until its owner Type is complete.
class Function {
 public:
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Scope& scope,
      Ttx::Documentation documentation,
      Perimortem::Utility::Range& source) -> Ttx::Function;
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Scope& scope,
      Ttx::Documentation documentation,
      Perimortem::Utility::Range& source,
      const Ttx::Type* owner,
      Bool& addressable) -> Ttx::Function;
  static auto evaluate_declaration(
      Ttx::Lexical::Cursor& cursor,
      Scope& scope,
      Ttx::Documentation documentation,
      const Ttx::Type* owner = nullptr) -> Ttx::Function;
};

}  // namespace Tetrodotoxin::Isa::Library
