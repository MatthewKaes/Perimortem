// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/layout.hpp"
#include "ttx/model/type.hpp"

namespace Ttx::Model {

// Expression is the Abstract contract for one evaluatable value. Expression
// identity remains distinct from Type identity so two values of the same Type
// remain distinct facts in the semantic DAG.
//
// get_type() returns the Type produced by the expression or Invalid when the
// source owner cannot establish one. get_inputs() exposes the ordered values
// required to evaluate the expression. The active ISA owns parsing, operator
// legality, and executable bodies while shared fitting and reflection consume
// these stable queries.
class Expression : public Concept::Abstract {
 public:
  using ContractOwner = Expression;
  static constexpr Perimortem::System::Uuid contract_id{
    0xb9716e09506c45e3,
    0x9537c9b4b327e108,
  };

  auto implements(Perimortem::System::Uuid requested) const -> Bool override {
    return requested == contract_id || Abstract::implements(requested);
  }

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Concept::Abstract& override {
    return get_type().resolve().resolve_context(route);
  }

  virtual auto get_type() const -> const Concept::Abstract& = 0;
  virtual auto get_inputs() const -> const Concept::Layout& = 0;

  // Ordinary expressions fit only their resolved Type. Constant domains may
  // extend this rule when their value proves a contextual conversion safe.
  virtual auto fits(const Type& target) const -> Bool {
    const Concept::Abstract& source_type = get_type().resolve();
    const Concept::Abstract& target_type = target.resolve();
    return source_type.is<Type>() && target_type.is<Type>() &&
           &source_type == &target_type;
  }
};

}  // namespace Ttx::Model
