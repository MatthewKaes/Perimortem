// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/library/scope.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Library {

// Function owns Library callable signatures and publishes Library blocks for
// function bodies.
class Function {
 public:
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Scope& scope,
      Ttx::Documentation documentation) -> Ttx::Type::Function;
  static auto evaluate_declaration(
      Ttx::Lexical::Cursor& cursor,
      Scope& scope,
      Ttx::Documentation documentation) -> Ttx::Type::Function;

 private:
  enum class BodyMode {
    Definition,
    Declaration,
  };

  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Scope& scope,
      Ttx::Documentation documentation,
      BodyMode body_mode) -> Ttx::Type::Function;
  static auto evaluate_body(
      Ttx::Lexical::Cursor& cursor,
      Scope& scope,
      Perimortem::Memory::Managed::Vector<Ttx::Type::Function::Block>& blocks)
      -> Bool;
};

}  // namespace Tetrodotoxin::Isa::Library
