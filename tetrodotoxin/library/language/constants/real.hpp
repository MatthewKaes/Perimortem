// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/constant.hpp"
#include "ttx/model/types/real.hpp"

namespace Tetrodotoxin::Library::Language::Constants {

// Real is an evaluated floating-point Constant. Source decimal text may remain
// a Dialect-owned literal Expression until a receiving Type selects a format,
// so constructing this contract never silently narrows an exact source literal.
// NaN values compare as one semantic value so Constant equality remains an
// equivalence relation suitable for Generic materialization keys.
class Real : public Constant {
 public:
  using ClassCatagory = Real;
  using Value = Real_64;
  static constexpr Perimortem::System::Uuid contract_id{
    0x4d64a697bce34b21,
    0xb9669826f970ad9b,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Constant::implements(requested);
  }

  constexpr auto equals(const Constant& rhs) const -> Bool override {
    return rhs.visit<Real>(
        [this, &rhs](const Real& selected) {
          if (!has_same_type(rhs)) {
            return False;
          }

          Value lhs_value = get_value();
          Value rhs_value = selected.get_value();
          return lhs_value == rhs_value || (__builtin_isnan(lhs_value) &&
                                            __builtin_isnan(rhs_value))
                     ? True
                     : False;
        },
        [](const Ttx::Concept::Abstract&) { return False; });
  }

  constexpr auto fits(const Ttx::Model::Type& target) const -> Bool override {
    const Ttx::Concept::Abstract& source_type = get_type().resolve();
    const Ttx::Concept::Abstract& target_type = target.resolve();
    return source_type.is<Ttx::Model::Types::Real>() &&
           target_type.is<Ttx::Model::Types::Real>() &&
           &source_type == &target_type;
  }

  virtual constexpr auto get_value() const -> Value = 0;
};

}  // namespace Tetrodotoxin::Library::Language::Constants
