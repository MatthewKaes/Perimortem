// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/library/scope.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Library {

// Function owns Library callable signatures and body token blocks.
class Function {
 public:
  static auto evaluate(
      Scope& scope,
      Ttx::Lexical::Cursor& cursor,
      Ttx::Documentation documentation) -> Ttx::Type::Function;
  static auto evaluate_declaration(
      Scope& scope,
      Ttx::Lexical::Cursor& cursor,
      Ttx::Documentation documentation) -> Ttx::Type::Function;

 private:
  enum class BodyMode {
    Definition,
    Declaration,
  };

  static auto evaluate(
      Scope& scope,
      Ttx::Lexical::Cursor& cursor,
      Ttx::Documentation documentation,
      BodyMode body_mode) -> Ttx::Type::Function;
  static auto evaluate_body(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Memory::Managed::Vector<Ttx::Type::Function::Block>& blocks)
      -> Bool;
};

}  // namespace Tetrodotoxin::Isa::Library
