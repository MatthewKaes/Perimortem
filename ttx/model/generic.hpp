// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "ttx/abstraction/abstract.hpp"
#include "ttx/model/argument.hpp"

namespace Ttx::Model {

// Generic is the Abstract contract for a named compile-time formula. It is not
// a Type and has no Layout. The containing source context owns formula lookup.
// Each concrete formula owns argument validation, materialization, and any
// cache of concrete Types it creates.
//
// Arguments already contain resolved semantic identities. A formula can use
// their ordered values directly as its cache key without alias walks, names,
// hashes, parent links, or another canonicalization interface. Failure returns
// Invalid while the source owner retains the diagnostic.
class Generic : public Abstraction::Abstract {
 public:
  using ContractOwner = Generic;
  static constexpr Perimortem::System::Uuid contract_id{
    0x8fe47e7b2c394bd7,
    0x9b3824546cc3bb50,
  };

  auto implements(Perimortem::System::Uuid requested) const -> Bool override {
    return requested == contract_id || Abstract::implements(requested);
  }

  virtual auto materialize(Perimortem::Core::View::Vector<Argument> arguments)
      const -> const Abstraction::Abstract& = 0;
};

}  // namespace Ttx::Model
