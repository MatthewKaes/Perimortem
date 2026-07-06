// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/isa/library/statement.hpp"
#include "ttx/block.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Library {

// Library::Block is the first executable Library body model.
//
// It is intentionally owned by the Library ISA, not by TTX. TTX records that a
// function has a represented body; Library records the statement sequence for
// that body.
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

  constexpr auto get_statements() const
      -> Perimortem::Core::View::Vector<Statement> {
    return statements;
  }

 private:
  inline static constexpr Ttx::Type representation =
      Ttx::Type("LibraryBlock"_view);

  Perimortem::Core::View::Vector<Statement> statements;
};

}  // namespace Tetrodotoxin::Isa::Library
