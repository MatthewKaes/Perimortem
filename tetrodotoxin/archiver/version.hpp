// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/static/vector.hpp"

namespace Tetrodotoxin::Archiver {

// Deterministic content stamp for a durable package table.
//
// This is a content hash, not a generated identity. Two independently built
// packages with the same serialized facts produce the same Version, allowing
// readers to match dependencies without relying on paths or load order.
class Version {
 public:
  constexpr Version() = default;
  constexpr Version(Bits_64 high, Bits_64 low)
      : high(high), low(low | Bits_64(1)) {}

  constexpr auto operator==(Version rhs) const -> Bool {
    return high == rhs.high && low == rhs.low;
  }

  constexpr auto operator!=(Version rhs) const -> Bool {
    return high != rhs.high || low != rhs.low;
  }

  constexpr auto operator<(Version rhs) const -> Bool {
    return high == rhs.high ? low < rhs.low : high < rhs.high;
  }

  constexpr auto get_value() const
      -> Perimortem::Core::Static::Vector<Bits_64, 2> {
    return {{high, low}};
  }

  constexpr auto is_set() const -> Bool { return high != 0 && low != 0; }

 private:
  Bits_64 high = 0;
  Bits_64 low = 0;
};

}  // namespace Tetrodotoxin::Archiver
