// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/layout.hpp"
#include "ttx/model/addressable.hpp"

namespace Ttx::Model {

// Callable is the Abstract contract for invocation. Its parameter and result
// Layouts are the target independent signature consumed by fitting,
// reflection, invocation, and lowering. Their concrete Value, Fluid, Named,
// Ranged, or Composite contracts own the fitting behavior. A Dialect may enrich
// the same Callable with an executable body, but the body is not part of this
// contract.
// Machine linkage and executable addresses belong to an ABI or execution
// contract. Callability alone does not imply an address to data.
class Callable : public Concept::Abstract {
 public:
  TTX_CONTRACT(Callable, Abstract, 0x01cc41c70a414b03, 0xacf5c601eccfd393);

  virtual constexpr auto get_parameters() const -> const Concept::Layout& = 0;
  virtual constexpr auto get_results() const -> const Concept::Layout& = 0;

  // A type-bound Callable carries its exact receiver as the reserved `self`
  // Addressable at parameter entry zero. The query derives that role from the
  // real parameter Layout rather than retaining a second marker.
  constexpr auto get_type_binding() const
      -> Perimortem::Core::Option<const Type&> {
    auto first = get_parameters().get_abstract(0);
    if (!first) {
      return {};
    }

    auto parameter = first->select<Addressable>();
    if (!parameter || parameter->get_name() != "self"_view) {
      return {};
    }
    return parameter->get_type();
  }

  constexpr auto is_type_bound() const -> Bool {
    return Bool(get_type_binding());
  }

  constexpr auto is_type_bound(const Type& receiver) const -> Bool {
    auto binding = get_type_binding();
    return binding && &*binding == &receiver;
  }
};

}  // namespace Ttx::Model
