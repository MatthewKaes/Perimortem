// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/compiler/execution/operand.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Compiler::Execution {

// A three-address operation with one SSA result.
class Binary {
 public:
  enum class Operator : Bits_8 {
    Add,
    Subtract,
    Multiply,
    Divide,
    Remainder,
    Equal,
  };

  constexpr Binary(
      Operator op,
      const Ttx::Type& operand_type,
      Operand left,
      Operand right,
      Addressable result)
      : op(op),
        operand_type(operand_type),
        left(left),
        right(right),
        result(result) {}

  constexpr auto get_operator() const -> Operator { return op; }
  constexpr auto get_operand_type() const -> const Ttx::Type& {
    return operand_type;
  }

  constexpr auto get_left() const -> const Operand& { return left; }

  constexpr auto get_right() const -> const Operand& { return right; }

  constexpr auto get_result() const -> Addressable { return result; }

 private:
  Operator op;
  const Ttx::Type& operand_type;
  Operand left;
  Operand right;
  Addressable result;
};

}  // namespace Tetrodotoxin::Compiler::Execution
