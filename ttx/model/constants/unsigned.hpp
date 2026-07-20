// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/constant.hpp"
#include "ttx/model/types/unsigned.hpp"

namespace Ttx::Model::Constants {

// Unsigned is the Constant contract for a non-negative integer value. The
// resolved Type supplies the authored width while the value remains wide enough
// to prove whether a narrower Unsigned target can represent it.
class Unsigned : public Constant {
 public:
  using ContractOwner = Unsigned;
  using Value = Unsigned_64;
  static constexpr Perimortem::System::Uuid contract_id{
    0xd48f7ac9d3454918,
    0xb2b28b158d5034d8,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Constant::implements(requested);
  }

  constexpr auto equals(const Constant& rhs) const -> Bool final {
    return rhs.is<Unsigned>() && has_same_type(rhs) &&
           get_value() == rhs.assume<Unsigned>().get_value();
  }

  constexpr auto fits(const Type& target) const -> Bool final {
    if (!get_type().resolve().is<Types::Unsigned>()) {
      return False;
    }

    const Concept::Abstract& target_type = target.resolve();
    if (!target_type.is<Types::Unsigned>()) {
      return False;
    }

    Count size = target_type.assume<Types::Unsigned>().get_size();
    if (size == 0) {
      return False;
    }
    if (size >= sizeof(Unsigned_64)) {
      return True;
    }

    return get_value() < (Unsigned_64(1) << (size * 8));
  }

  virtual constexpr auto get_value() const -> Value = 0;
};

}  // namespace Ttx::Model::Constants
