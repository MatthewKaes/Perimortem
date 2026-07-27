// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/layout.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language {

// Expression is the Abstract contract for one evaluatable value. Expression
// identity remains distinct from Type identity so two values of the same Type
// remain distinct facts in the semantic DAG.
//
// get_type() returns the Type produced by the expression or Invalid when the
// source owner cannot establish one. get_inputs() exposes the ordered values
// required to evaluate the expression. Library owns parsing, operator
// legality, executable bodies, and value fitting while reflection consumes
// these stable queries.
class Expression : public Ttx::Concept::Abstract {
 public:
  using ClassCatagory = Expression;
  static constexpr Perimortem::System::Uuid contract_id{
    0xb9716e09506c45e3,
    0x9537c9b4b327e108,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id ||
           Ttx::Concept::Abstract::implements(requested);
  }

  constexpr auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override {
    return get_type().resolve().resolve_context(route);
  }

  virtual constexpr auto get_type() const -> const Ttx::Concept::Abstract& = 0;
  virtual constexpr auto get_inputs() const -> const Ttx::Concept::Layout& = 0;

  // Ordinary expressions fit only their resolved Type. Constant domains may
  // extend this rule when their value proves a contextual conversion safe.
  virtual constexpr auto fits(const Ttx::Model::Type& target) const -> Bool {
    const Ttx::Concept::Abstract& source_type = get_type().resolve();
    const Ttx::Concept::Abstract& target_type = target.resolve();
    return source_type.is<Ttx::Model::Type>() &&
           target_type.is<Ttx::Model::Type>() && &source_type == &target_type;
  }
};

}  // namespace Tetrodotoxin::Library::Language
