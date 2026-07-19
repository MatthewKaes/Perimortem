// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/constant.hpp"
#include "ttx/model/types/real.hpp"

namespace Ttx::Model::Constants {

// Real is an evaluated floating-point Constant. Source decimal text may remain
// an ISA-owned literal Expression until a receiving Type selects a format, so
// constructing this contract never silently narrows an exact source literal.
// NaN values compare as one semantic value so Constant equality remains an
// equivalence relation suitable for Generic argument caches.
class Real : public Constant {
 public:
  using ContractOwner = Real;
  using Value = Real_64;
  static constexpr Perimortem::System::Uuid contract_id{
    0x4d64a697bce34b21,
    0xb9669826f970ad9b,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Constant::implements(requested);
  }

  constexpr auto equals(const Constant& rhs) const -> Bool final {
    if (!rhs.is<Real>() || !has_same_type(rhs)) {
      return False;
    }

    Value lhs_value = get_value();
    Value rhs_value = rhs.as<Real>().get_value();
    return lhs_value == rhs_value ||
           (__builtin_isnan(lhs_value) && __builtin_isnan(rhs_value));
  }

  constexpr auto fits(const Type& target) const -> Bool final {
    const Concept::Abstract& source_type = get_type().resolve();
    const Concept::Abstract& target_type = target.resolve();
    return source_type.is<Types::Real>() && target_type.is<Types::Real>() &&
           &source_type == &target_type;
  }

  virtual constexpr auto get_value() const -> Value = 0;
};

}  // namespace Ttx::Model::Constants
