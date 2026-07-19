// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/constant.hpp"
#include "ttx/model/types/flag.hpp"

namespace Ttx::Model::Constants {

// Flag is the Constant contract for a binary logical value. Either value fits
// every resolved Flag Type regardless of the toolchain's chosen storage width.
class Flag : public Constant {
 public:
  using ContractOwner = Flag;
  using Value = Bool;
  static constexpr Perimortem::System::Uuid contract_id{
    0x09d395cb5fea4765,
    0x897a36a658c2486a,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Constant::implements(requested);
  }

  constexpr auto equals(const Constant& rhs) const -> Bool final {
    return rhs.is<Flag>() && has_same_type(rhs) &&
           get_value() == rhs.as<Flag>().get_value();
  }

  constexpr auto fits(const Type& target) const -> Bool final {
    return get_type().resolve().is<Types::Flag>() &&
           target.resolve().is<Types::Flag>();
  }

  virtual constexpr auto get_value() const -> Value = 0;
};

}  // namespace Ttx::Model::Constants
