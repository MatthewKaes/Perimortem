// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/abi/linkage.hpp"
#include "tetrodotoxin/compiler/execution/binary.hpp"
#include "tetrodotoxin/compiler/execution/body.hpp"
#include "tetrodotoxin/compiler/execution/constant.hpp"
#include "tetrodotoxin/compiler/execution/operand.hpp"
#include "tetrodotoxin/compiler/execution/operation.hpp"
#include "ttx/function.hpp"

namespace Tetrodotoxin::Compiler::Execution {

// Builder is the only mutable phase of a compiler Body.
//
// It assigns body-local SSA identities monotonically, records three-address
// operations, and freezes the final views into the transaction arena. Failed
// operations do not publish partial instructions, so the returned Body is
// always structurally complete and immutable.
class Builder {
 public:
  Builder(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Function& signature);

  auto parameter(const Ttx::Member& member) const -> Operand;
  static auto constant(Constant value) -> Operand {
    return Operand(Perimortem::Core::Data::take(value));
  }

  auto binary(
      Binary::Operator op,
      const Ttx::Type& operand_type,
      const Ttx::Type& result_type,
      Operand left,
      Operand right) -> Operand;
  auto call(
      const Tetrodotoxin::Abi::Linkage& linkage,
      Perimortem::Core::View::Vector<Operand> arguments)
      -> Perimortem::Utility::Range;
  auto return_values(Perimortem::Core::View::Vector<Operand> values = {})
      -> Bool;
  auto finish() -> const Body*;

  constexpr auto is_terminated() const -> Bool { return terminated; }

 private:
  auto binding_type(const Operand& operand) -> const Ttx::Type*;
  auto fold_binary(
      Binary::Operator op,
      const Operand& left,
      const Operand& right) const -> Operand;
  auto append_operands(Perimortem::Core::View::Vector<Operand> source)
      -> Perimortem::Utility::Range;

  Perimortem::Memory::Allocator::Arena& arena;
  const Ttx::Function& signature;
  Perimortem::Memory::Managed::Vector<Operation> operations;
  Perimortem::Memory::Managed::Vector<Operand> operands;
  Perimortem::Memory::Managed::Vector<Binding> bindings;
  Bool terminated = False;
};

}  // namespace Tetrodotoxin::Compiler::Execution
