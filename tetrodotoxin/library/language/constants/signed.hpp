// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/constant.hpp"
#include "ttx/model/types/signed.hpp"

namespace Tetrodotoxin::Library::Language::Constants {

// Signed is the Constant contract for a signed integer value. Its resolved Type
// remains part of identity while fitting may prove that the value is in range
// for another Signed width.
class Signed : public Constant {
 public:
  using ClassCatagory = Signed;
  using Value = Signed_64;
  static constexpr Perimortem::System::Uuid contract_id{
    0xb7e0f0e5d1b44874,
    0x9361ac25cc53d15e,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Constant::implements(requested);
  }

  constexpr auto equals(const Constant& rhs) const -> Bool override {
    return rhs.visit<Signed>(
        [this, &rhs](const Signed& selected) {
          return has_same_type(rhs) && get_value() == selected.get_value()
                     ? True
                     : False;
        },
        [](const Ttx::Concept::Abstract&) { return False; });
  }

  constexpr auto fits(const Ttx::Model::Type& target) const -> Bool override {
    if (!get_type().resolve().is<Ttx::Model::Types::Signed>()) {
      return False;
    }

    const Ttx::Concept::Abstract& target_type = target.resolve();
    return target_type.visit<Ttx::Model::Types::Signed>(
        [this](const Ttx::Model::Types::Signed& selected) {
          Count size = selected.get_size();
          if (size == 0) {
            return False;
          }

          if (size >= sizeof(Signed_64)) {
            return True;
          }

          Signed_64 limit = Signed_64(1) << (size * 8 - 1);
          return get_value() >= -limit && get_value() < limit ? True : False;
        },
        [](const Ttx::Concept::Abstract&) { return False; });
  }

  virtual constexpr auto get_value() const -> Value = 0;
};

}  // namespace Tetrodotoxin::Library::Language::Constants
