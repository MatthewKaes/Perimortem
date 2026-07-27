// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/expression.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/layouts/fluid.hpp"

namespace Tetrodotoxin::Library::Language {

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
class Binding : public Expression {
 public:
  using ClassCatagory = Binding;
  static constexpr Perimortem::System::Uuid contract_id{
    0xdaec51f766004747,
    0xbabe4b64c2c7b77c,
  };

  constexpr Binding(
      Perimortem::Core::View::Bytes name,
      const Expression& expression)
      : name(name),
        expression(expression),
        input(expression),
        inputs({&this->input, 1}) {}

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Expression::implements(requested);
  }
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return name;
  }
  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return Ttx::Concept::Documentation::get_empty();
  }
  constexpr auto get_type() const -> const Ttx::Concept::Abstract& override {
    return get_expression().get_type();
  }
  constexpr auto get_inputs() const -> const Ttx::Concept::Layout& override {
    return inputs;
  }
  constexpr auto fits(const Ttx::Model::Type& target) const -> Bool override {
    return get_expression().fits(target);
  }

  constexpr auto get_expression() const -> const Expression& {
    return expression;
  }

 private:
  Perimortem::Core::View::Bytes name;
  const Expression& expression;
  Ttx::Concept::Reference<Ttx::Concept::Abstract> input;
  Ttx::Model::Layouts::Fluid inputs;
};

}  // namespace Tetrodotoxin::Library::Language
