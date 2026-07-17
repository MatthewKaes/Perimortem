// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/constant.hpp"
#include "ttx/model/types/signed.hpp"

namespace Ttx::Model::Constants {

// Signed is the Constant contract for a signed integer value. Its resolved Type
// remains part of identity while fitting may prove that the value is in range
// for another Signed width.
class Signed : public Constant {
 public:
  using ContractOwner = Signed;
  using Value = Signed_64;
  static constexpr Perimortem::System::Uuid contract_id{
    0xb7e0f0e5d1b44874,
    0x9361ac25cc53d15e,
  };

  auto implements(Perimortem::System::Uuid requested) const -> Bool override {
    return requested == contract_id || Constant::implements(requested);
  }

  auto equals(const Constant& rhs) const -> Bool final {
    return rhs.is<Signed>() && has_same_type(rhs) &&
           get_value() == rhs.as<Signed>().get_value();
  }

  auto fits(const Type& target) const -> Bool final {
    if (!get_type().resolve().is<Types::Signed>()) {
      return False;
    }

    const Concept::Abstract& target_type = target.resolve();
    if (!target_type.is<Types::Signed>()) {
      return False;
    }

    Count size = target_type.as<Types::Signed>().get_size();
    if (size == 0) {
      return False;
    }
    if (size >= sizeof(Signed_64)) {
      return True;
    }

    Signed_64 limit = Signed_64(1) << (size * 8 - 1);
    return get_value() >= -limit && get_value() < limit;
  }

  virtual auto get_value() const -> Value = 0;
};

}  // namespace Ttx::Model::Constants
