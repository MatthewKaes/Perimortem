// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/isa/shader/statement.hpp"

namespace Tetrodotoxin::Isa::Shader {

// Shader::Block is the first executable function model for shader ouput.
//
// A block is durable Shader data that is used with TTX::Type data to emit
// actual SPIR-V commands.
class Block {
 public:
  explicit constexpr Block(Perimortem::Core::View::Vector<Statement> statements)
      : statements(statements) {}

  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Isa::Base::Context& context) -> const Block*;

  constexpr auto get_statements() const
      -> Perimortem::Core::View::Vector<Statement> {
    return statements;
  }

 private:
  Perimortem::Core::View::Vector<Statement> statements;
};

}  // namespace Tetrodotoxin::Isa::Shader
