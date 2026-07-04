// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

namespace Ttx {

// Attribute is the small metadata side channel for facts that belong to a
// source identity but are owned by a layer above the core Type model.
class Attribute {
 public:
  constexpr Attribute() = default;
  constexpr Attribute(
      Perimortem::Core::View::Bytes key,
      Perimortem::Core::View::Bytes value)
      : key(key), value(value) {}

  constexpr auto get_key() const -> Perimortem::Core::View::Bytes {
    return key;
  }
  constexpr auto get_value() const -> Perimortem::Core::View::Bytes {
    return value;
  }
  constexpr auto is_empty() const -> Bool {
    return key.is_empty() && value.is_empty();
  }

 private:
  Perimortem::Core::View::Bytes key;
  Perimortem::Core::View::Bytes value;
};

}  // namespace Ttx
