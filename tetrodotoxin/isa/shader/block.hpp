// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/context.hpp"
#include "tetrodotoxin/isa/shader/statement.hpp"
#include "ttx/block.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Shader {

class Block : public Ttx::Block {
 public:
  explicit constexpr Block(
      Perimortem::Core::View::Vector<Statement> statements)
      : Ttx::Block(get_representation_type()), statements(statements) {}

  static constexpr auto get_representation_type() -> const Ttx::Type& {
    return representation;
  }

  static constexpr auto from(const Ttx::Type::Function::Block& block)
      -> const Block* {
    const Ttx::Block* body = block.get_block();
    if (body == nullptr ||
        body->get_representation() != &get_representation_type()) {
      return nullptr;
    }

    return static_cast<const Block*>(body);
  }

  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Isa::Context& context,
      Perimortem::Memory::Managed::Vector<Ttx::Type::Function::Block>& blocks)
      -> Bool;

  constexpr auto get_statements() const
      -> Perimortem::Core::View::Vector<Statement> {
    return statements;
  }

 private:
  inline static constexpr Ttx::Type representation =
      Ttx::Type("ShaderBlock"_view);

  Perimortem::Core::View::Vector<Statement> statements;
};

}  // namespace Tetrodotoxin::Isa::Shader
