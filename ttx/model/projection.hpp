// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/reference.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/expression.hpp"
#include "ttx/model/layouts/fluid.hpp"

namespace Ttx::Model {

// Projection is the Expression that selects one real Addressable from
// one receiver Expression. It preserves its own identity, publishes the
// Addressable's resolved object through get_type(), and retains the receiver as
// its input Layout.
//
// The evaluator resolves names and validates receiver shape before constructing
// a Projection. If the selected Addressable later resolves to Invalid, ordinary
// Expression queries propagate that failure without trapping.
//
// The input Layout borrows the stored receiver Reference. The graph owner
// constructs a Projection at its final stable address and keeps both borrowed
// semantic objects alive for the same interval.
class Projection final : public Expression {
 public:
  using ContractOwner = Projection;
  static constexpr Perimortem::System::Uuid contract_id{
    0xdf9d470a5b024335,
    0xb38fc893710f519e,
  };

  Projection(const Expression& receiver, const Addressable& addressable);

  using Expression::fits;

  auto implements(Perimortem::System::Uuid requested) const -> Bool override;
  auto get_name() const -> Perimortem::Core::View::Bytes override;
  auto get_type() const -> const Concept::Abstract& override;
  auto get_inputs() const -> const Concept::Layout& override;

  auto get_receiver() const -> const Expression&;
  constexpr auto get_addressable() const -> const Addressable& {
    return addressable;
  }

 private:
  Concept::Reference<Concept::Abstract> receiver;
  const Addressable& addressable;
  Layouts::Fluid inputs;
};

}  // namespace Ttx::Model
