// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/expression.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/layouts/fluid.hpp"

namespace Tetrodotoxin::Library::Language {

// Projection is the Expression that selects one real Addressable from
// one receiver Expression. It preserves its own identity, publishes the
// Addressable's resolved Type through get_type(), and retains the receiver as
// its input Layout.
//
// Library resolves names and validates receiver shape before constructing a
// Projection. If the selected Addressable's Type later resolves to Invalid,
// ordinary Expression queries propagate that failure without trapping.
//
// The input Layout borrows the stored receiver Reference. The graph owner
// constructs a Projection at its final stable address and keeps both borrowed
// semantic objects alive for the same interval.
class Projection : public Expression {
 public:
  using ClassCatagory = Projection;
  static constexpr Perimortem::System::Uuid contract_id{
    0xdf9d470a5b024335,
    0xb38fc893710f519e,
  };

  constexpr Projection(
      const Expression& receiver,
      const Ttx::Model::Addressable& addressable)
      : receiver(receiver),
        addressable(addressable),
        input(receiver),
        inputs({&this->input, 1}) {}

  using Expression::fits;

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Expression::implements(requested);
  }
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return addressable.get_name();
  }
  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return addressable.get_documentation();
  }
  constexpr auto get_type() const -> const Ttx::Model::Type& override {
    return addressable.get_type();
  }
  constexpr auto get_inputs() const -> const Ttx::Concept::Layout& override {
    return inputs;
  }

  constexpr auto get_receiver() const -> const Expression& { return receiver; }
  constexpr auto get_addressable() const -> const Ttx::Model::Addressable& {
    return addressable;
  }

 private:
  const Expression& receiver;
  const Ttx::Model::Addressable& addressable;
  Ttx::Concept::Reference<Ttx::Concept::Abstract> input;
  Ttx::Model::Layouts::Fluid inputs;
};

}  // namespace Tetrodotoxin::Library::Language
