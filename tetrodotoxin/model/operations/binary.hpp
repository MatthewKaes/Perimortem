// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/bodies/operations/binary.hpp"

namespace Tetrodotoxin::Model::Operations {

// Binary owns Tetrodotoxin's arithmetic bytecode vocabulary. Individual
// Dialects decide which operand Type combinations are legal and what result
// Type they produce. The common TTX operation owns only the two-operand shape.
class Binary {
 public:
  enum class Operator : Unsigned_8 {
    Add = 1,
    Subtract = 2,
    Multiply = 3,
    Divide = 4,
  };

  static constexpr auto is_valid(
      const Ttx::Model::Bodies::Operations::Binary& operation) -> Bool {
    return operation.get_bytecode() >= Unsigned_8(Operator::Add) &&
           operation.get_bytecode() <= Unsigned_8(Operator::Divide);
  }

  static constexpr auto get_operator(
      const Ttx::Model::Bodies::Operations::Binary& operation) -> Operator {
    return Operator(operation.get_bytecode());
  }

  static constexpr auto get_bytecode(Operator operation) -> Unsigned_8 {
    return Unsigned_8(operation);
  }
};

}  // namespace Tetrodotoxin::Model::Operations
