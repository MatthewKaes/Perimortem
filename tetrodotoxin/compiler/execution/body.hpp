// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/compiler/execution/binding.hpp"
#include "tetrodotoxin/compiler/execution/block.hpp"
#include "tetrodotoxin/compiler/execution/operand.hpp"
#include "tetrodotoxin/compiler/execution/operation.hpp"

namespace Tetrodotoxin::Compiler::Execution {

// Immutable execution data for one TTX function. Operations preserve authored
// order while bindings give parameters and computed values stable SSA
// identities.
class Body {
 public:
  constexpr Body(
      Perimortem::Core::View::Vector<Block> blocks,
      Perimortem::Core::View::Vector<Operation> operations,
      Perimortem::Core::View::Vector<Operand> operands,
      Perimortem::Core::View::Vector<Binding> bindings)
      : blocks(blocks),
        operations(operations),
        operands(operands),
        bindings(bindings) {}

  constexpr auto get_blocks() const -> Perimortem::Core::View::Vector<Block> {
    return blocks;
  }

  constexpr auto get_operations() const
      -> Perimortem::Core::View::Vector<Operation> {
    return operations;
  }

  constexpr auto get_operands() const
      -> Perimortem::Core::View::Vector<Operand> {
    return operands;
  }

  constexpr auto get_bindings() const
      -> Perimortem::Core::View::Vector<Binding> {
    return bindings;
  }

  template <typename Type>
  auto count() const -> Count {
    Count result = 0;
    for (Count i = 0; i < operations.get_size(); i++) {
      result += operations[i].template find<Type>() != nullptr ? 1 : 0;
    }

    return result;
  }

  template <typename Type>
  auto find(Count index = 0) const -> const Type* {
    for (Count i = 0; i < operations.get_size(); i++) {
      const Type* value = operations[i].template find<Type>();
      if (value != nullptr && index-- == 0) {
        return value;
      }
    }

    return nullptr;
  }

 private:
  Perimortem::Core::View::Vector<Block> blocks;
  Perimortem::Core::View::Vector<Operation> operations;
  Perimortem::Core::View::Vector<Operand> operands;
  Perimortem::Core::View::Vector<Binding> bindings;
};

}  // namespace Tetrodotoxin::Compiler::Execution
