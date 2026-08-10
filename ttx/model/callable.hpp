// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/layout.hpp"

namespace Ttx::Model {

// Callable is the Abstract contract for invocation. Its parameter and result
// Layouts are the target-independent signature consumed by fitting,
// reflection, invocation, and lowering. Their concrete Fluid, Named, Ranged,
// or Composite contracts own the fitting behavior. A Dialect may enrich the
// same Callable with an executable body, but the body is not part of this
// contract.
// Machine linkage and executable addresses belong to an ABI or execution
// contract. Callability alone does not imply an address to data.
class Callable : public Concept::Abstract {
 public:
  TTX_CONTRACT(Callable, Abstract, 0x01cc41c70a414b03, 0xacf5c601eccfd393);

  virtual constexpr auto get_parameters() const -> const Concept::Layout& = 0;
  virtual constexpr auto get_results() const -> const Concept::Layout& = 0;
};

}  // namespace Ttx::Model
