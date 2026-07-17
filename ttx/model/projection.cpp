// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/projection.hpp"

Ttx::Model::Projection::Projection(
    const Expression& receiver,
    const Addressable& addressable)
    : receiver(receiver),
      addressable(addressable),
      inputs({&this->receiver, 1}) {}

auto Ttx::Model::Projection::implements(
    Perimortem::System::Uuid requested) const -> Bool {
  return requested == contract_id || Expression::implements(requested);
}

auto Ttx::Model::Projection::get_name() const -> Perimortem::Core::View::Bytes {
  return addressable.get_name();
}

auto Ttx::Model::Projection::get_type() const -> const Concept::Abstract& {
  return addressable.resolve();
}

auto Ttx::Model::Projection::get_inputs() const -> const Concept::Layout& {
  return inputs;
}

auto Ttx::Model::Projection::get_receiver() const -> const Expression& {
  return receiver.get().as<Expression>();
}
