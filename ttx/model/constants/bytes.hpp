// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "ttx/model/constant.hpp"

namespace Ttx::Model::Constants {

// Bytes is the Constant contract for immutable byte-array data. Quoted source,
// hexadecimal byte literals, and embedded files may all produce this value.
// TTX defines no native String constant. A language may materialize its own
// String Type from these bytes through an ordinary Callable. The implementing
// graph owner keeps the immutable backing storage alive for the Constant.
class Bytes : public Constant {
 public:
  using ContractOwner = Bytes;
  using Value = Perimortem::Core::View::Bytes;
  static constexpr Perimortem::System::Uuid contract_id{
    0x7ca3807fea7d4c28,
    0xb723464db6c29666,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Constant::implements(requested);
  }

  constexpr auto equals(const Constant& rhs) const -> Bool final {
    return rhs.is<Bytes>() && has_same_type(rhs) &&
           get_value() == rhs.assume<Bytes>().get_value();
  }

  virtual constexpr auto get_value() const -> Value = 0;
};

}  // namespace Ttx::Model::Constants
