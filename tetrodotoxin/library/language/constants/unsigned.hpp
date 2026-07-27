// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/constant.hpp"
#include "ttx/model/types/unsigned.hpp"

namespace Tetrodotoxin::Library::Language::Constants {

// Unsigned is the Constant contract for a non-negative integer value. The
// resolved Type supplies the authored width while the value remains wide enough
// to prove whether a narrower Unsigned target can represent it.
class Unsigned : public Constant {
 public:
  using ClassCatagory = Unsigned;
  using Value = Unsigned_64;
  static constexpr Perimortem::System::Uuid contract_id{
    0xd48f7ac9d3454918,
    0xb2b28b158d5034d8,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Constant::implements(requested);
  }

  constexpr auto equals(const Constant& rhs) const -> Bool override {
    return rhs.visit<Unsigned>(
        [this, &rhs](const Unsigned& selected) {
          return has_same_type(rhs) && get_value() == selected.get_value()
                     ? True
                     : False;
        },
        [](const Ttx::Concept::Abstract&) { return False; });
  }

  constexpr auto fits(const Ttx::Model::Type& target) const -> Bool override {
    if (!get_type().resolve().is<Ttx::Model::Types::Unsigned>()) {
      return False;
    }

    const Ttx::Concept::Abstract& target_type = target.resolve();
    return target_type.visit<Ttx::Model::Types::Unsigned>(
        [this](const Ttx::Model::Types::Unsigned& selected) {
          Count size = selected.get_size();
          if (size == 0) {
            return False;
          }

          if (size >= sizeof(Unsigned_64)) {
            return True;
          }

          return get_value() < (Unsigned_64(1) << (size * 8)) ? True : False;
        },
        [](const Ttx::Concept::Abstract&) { return False; });
  }

  virtual constexpr auto get_value() const -> Value = 0;
};

}  // namespace Tetrodotoxin::Library::Language::Constants
