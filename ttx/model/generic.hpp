// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/layout.hpp"

namespace Ttx::Model {

// Generic is the Abstract contract for a named compile-time formula. It is not
// a Type and has no Layout. The containing source context owns formula lookup.
// Each concrete formula owns argument validation, materialization, and any
// cache of concrete Types it creates.
//
// Arguments are real compile-time Abstracts carried by an identity-free
// ordered Layout. Types are therefore first-class formula inputs, while scalar
// values remain real Constant Abstracts with their domain, Type, and payload
// intact. Generic owns the cache equivalence rule so no argument like wrapper
// can become a second semantic representation.
//
// Failure returns Invalid while the source owner retains the diagnostic.
class Generic : public Concept::Abstract {
 public:
  using ContractOwner = Generic;
  static constexpr Perimortem::System::Uuid contract_id{
    0x8fe47e7b2c394bd7,
    0x9b3824546cc3bb50,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Abstract::implements(requested);
  }

  virtual auto materialize(const Concept::Layout& arguments) const
      -> const Concept::Abstract& = 0;

 protected:
  // Resolution removes authored aliases and redirections. Constants with
  // distinct object identities still denote the same compile-time input when
  // their contracts prove equal; all other Abstracts retain object identity.
  static auto arguments_equal(
      const Concept::Layout& lhs,
      const Concept::Layout& rhs) -> Bool;
};

}  // namespace Ttx::Model
