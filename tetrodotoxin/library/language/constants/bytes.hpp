// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/library/language/constant.hpp"

namespace Tetrodotoxin::Library::Language::Constants {

// Bytes is the Constant contract for immutable byte-array data. Quoted source,
// hexadecimal byte literals, and embedded files may all produce this value.
// Library defines no native String constant. A concrete owner may materialize
// its own String Type from these bytes through an ordinary Callable. The graph
// owner keeps the immutable backing storage alive for the Constant.
class Bytes : public Constant {
 public:
  using ClassCatagory = Bytes;
  using Value = Perimortem::Core::View::Bytes;
  static constexpr Perimortem::System::Uuid contract_id{
    0x7ca3807fea7d4c28,
    0xb723464db6c29666,
  };

  constexpr Bytes(const Ttx::Model::Type& type, Value value)
      : type(type), value(value) {}

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Constant::implements(requested);
  }

  constexpr auto get_type() const -> const Ttx::Model::Type& override {
    return type;
  }

  virtual constexpr auto get_value() const -> Value { return value; }

  constexpr auto equals(const Constant& rhs) const -> Bool override {
    return rhs.visit<Bytes>(
        [this, &rhs](const Bytes& selected) {
          return has_same_type(rhs) && get_value() == selected.get_value()
                     ? ::True
                     : ::False;
        },
        [](const Ttx::Concept::Abstract&) { return ::False; });
  }

 private:
  const Ttx::Model::Type& type;
  Value value;
};

}  // namespace Tetrodotoxin::Library::Language::Constants
