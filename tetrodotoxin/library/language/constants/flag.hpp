// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/constant.hpp"
#include "ttx/model/types/flag.hpp"

namespace Tetrodotoxin::Library::Language::Constants {

// Flag is the Constant contract for a binary logical value. Either value fits
// every resolved Flag Type regardless of the toolchain's chosen storage width.
class Flag : public Constant {
 public:
  using ClassCatagory = Flag;
  using Value = Bool;
  static constexpr Perimortem::System::Uuid contract_id{
    0x09d395cb5fea4765,
    0x897a36a658c2486a,
  };

  constexpr Flag(const Ttx::Model::Types::Flag& type, Value value)
      : type(type), value(value) {}

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Constant::implements(requested);
  }

  constexpr auto get_type() const -> const Ttx::Model::Types::Flag& override {
    return type;
  }

  virtual constexpr auto get_value() const -> Value { return value; }

  constexpr auto equals(const Constant& rhs) const -> Bool override {
    return rhs.visit<Flag>(
        [this, &rhs](const Flag& selected) {
          return has_same_type(rhs) && get_value() == selected.get_value()
                     ? ::True
                     : ::False;
        },
        [](const Ttx::Concept::Abstract&) { return ::False; });
  }

  constexpr auto fits(const Ttx::Model::Type& target) const -> Bool override {
    return get_type().resolve().is<Ttx::Model::Types::Flag>() &&
           target.resolve().is<Ttx::Model::Types::Flag>();
  }

 private:
  const Ttx::Model::Types::Flag& type;
  Value value;
};

}  // namespace Tetrodotoxin::Library::Language::Constants
