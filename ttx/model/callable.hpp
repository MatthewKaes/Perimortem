// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/layout.hpp"

namespace Ttx::Model {

// Callable is the Abstract contract for invocation. Its parameter and result
// Layouts are the target-independent signature consumed by fitting,
// reflection, invocation, and lowering. Their concrete Fluid, Named, or
// Structured contracts own the fitting behavior. An ISA may enrich the same
// Callable with an executable body, but the body is not part of this contract.
//
// get_address() is total even before linkage. It returns a resolved or
// explicitly unresolved Addressable Abstract, or Invalid when the query
// failed. It never returns a nullable pointer and Callable never manufactures
// target address or calling-convention facts itself.
class Callable : public Concept::Abstract {
 public:
  using ContractOwner = Callable;
  static constexpr Perimortem::System::Uuid contract_id{
    0x01cc41c70a414b03,
    0xacf5c601eccfd393,
  };

  auto implements(Perimortem::System::Uuid requested) const -> Bool override {
    return requested == contract_id || Abstract::implements(requested);
  }

  virtual auto get_parameters() const -> const Concept::Layout& = 0;
  virtual auto get_results() const -> const Concept::Layout& = 0;
  virtual auto get_address() const -> const Concept::Abstract& = 0;
};

}  // namespace Ttx::Model
