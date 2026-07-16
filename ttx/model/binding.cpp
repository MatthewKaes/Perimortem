// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/binding.hpp"

Ttx::Model::Binding::Binding(
    Perimortem::Core::View::Bytes name,
    const Expression& expression)
    : name(name), expression(expression), inputs({&this->expression, 1}) {}

auto Ttx::Model::Binding::implements(Perimortem::System::Uuid requested) const
    -> Bool {
  return requested == contract_id || Expression::implements(requested);
}

auto Ttx::Model::Binding::get_name() const -> Perimortem::Core::View::Bytes {
  return name;
}

auto Ttx::Model::Binding::get_type() const -> const Abstraction::Abstract& {
  return get_expression().get_type();
}

auto Ttx::Model::Binding::get_inputs() const -> const Layout& {
  return inputs;
}

auto Ttx::Model::Binding::fits(const Type& target) const -> Bool {
  return get_expression().fits(target);
}

auto Ttx::Model::Binding::get_expression() const -> const Expression& {
  return expression.get().as<Expression>();
}
