// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/reference.hpp"
#include "ttx/model/expression.hpp"
#include "ttx/model/layouts/fluid.hpp"

namespace Ttx::Model {

// Binding is the Expression that gives one value an authored name. It
// is the entry used by Named value flow and is not a name-resolution phase,
// symbol table, binder, storage declaration, or ownership edge.
//
// Binding preserves its own identity, delegates Type fitting to the underlying
// Expression, and retains that Expression in its input Layout. An empty or
// duplicate name is rejected by Named Layout fitting.
//
// The input Layout borrows the stored Reference below. The graph owner
// constructs a Binding at its final stable address like every other semantic
// identity and keeps the underlying Expression alive for the same interval.
class Binding final : public Expression {
 public:
  using ContractOwner = Binding;
  static constexpr Perimortem::System::Uuid contract_id{
    0xdaec51f766004747,
    0xbabe4b64c2c7b77c,
  };

  Binding(Perimortem::Core::View::Bytes name, const Expression& expression);

  auto implements(Perimortem::System::Uuid requested) const -> Bool override;
  auto get_name() const -> Perimortem::Core::View::Bytes override;
  auto get_type() const -> const Concept::Abstract& override;
  auto get_inputs() const -> const Concept::Layout& override;
  auto fits(const Type& target) const -> Bool override;

  auto get_expression() const -> const Expression&;

 private:
  Perimortem::Core::View::Bytes name;
  Concept::Reference<Concept::Abstract> expression;
  Layouts::Fluid inputs;
};

}  // namespace Ttx::Model
