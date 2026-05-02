// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/data.hpp"
#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"

namespace Perimortem::System {

class Uuid {
  static constexpr auto ascii_to_nibble(Byte byte) -> Bits_64 {
    switch (byte) {
      case '0' ... '9':
        return byte - '0';
      case 'a' ... 'f':
        return byte - 'a' + 10;
      case 'A' ... 'F':
        return byte - 'A' + 10;
      default:
        return 0;
    }
  }

 public:
  constexpr Uuid() = default;

  constexpr Uuid(const Uuid& rhs) : low_high(rhs.low_high) {}

  // Big endian formatting (highest byte to lowest byte)
  explicit constexpr Uuid(const Core::Static::Bytes<32>& source) {
    if consteval {
      for (Count i = 0; i < 16; i++) {
        low_high[0] |= ascii_to_nibble(source[i + 16]) << (60 - i * 4);
      }

      for (Count i = 0; i < 16; i++) {
        low_high[1] |= ascii_to_nibble(source[i]) << (60 - i * 4);
      }
    } else {
      deserialize(source);
    }
  }

  // Expects values in the order of [High][Low]
  explicit constexpr Uuid(Bits_64 high, Bits_64 low) {
    low_high[0] = low;
    low_high[1] = high;
  }

  explicit constexpr Uuid(const Core::Static::Bytes<36>& source) {
    if consteval {
      // Low 12 block
      for (Count i = 0; i < 12; i++) {
        low_high[0] |= ascii_to_nibble(source[i + 24]) << (44 - i * 4);
      }

      // Lowest 4 block
      for (Count i = 0; i < 4; i++) {
        low_high[0] |= ascii_to_nibble(source[i + 19]) << (60 - i * 4);
      }

      // Mid 4 block
      for (Count i = 0; i < 4; i++) {
        low_high[1] |= ascii_to_nibble(source[i + 14]) << (12 - i * 4);
      }

      // Highest 4 block
      for (Count i = 0; i < 4; i++) {
        low_high[1] |= ascii_to_nibble(source[i + 9]) << (28 - i * 4);
      }

      // High 8 block
      for (Count i = 0; i < 8; i++) {
        low_high[1] |= ascii_to_nibble(source[i]) << (60 - i * 4);
      }
    } else {
      deserialize(source);
    }
  }

  constexpr auto operator==(const Uuid& rhs) const -> Bool {
    return low_high[0] == rhs.low_high[0] && low_high[1] == rhs.low_high[1];
  }

  constexpr auto operator!=(const Uuid& rhs) const -> Bool {
    return low_high[0] != rhs.low_high[0] && low_high[1] != rhs.low_high[1];
  }

  constexpr auto operator<(const Uuid& rhs) const -> Bool {
    return low_high[1] < rhs.low_high[1] && low_high[0] < rhs.low_high[0];
  }

  constexpr auto get_value() const -> const Core::Static::Vector<Bits_64, 2> {
    Core::Static::Vector<Bits_64, 2> high_low;
    high_low[0] = low_high[1];
    high_low[1] = low_high[0];
    return high_low;
  }

  constexpr auto is_set() const -> Bool {
    return low_high[0] != 0 && low_high[1] != 0;
  }

  auto deserialize(const Core::Static::Bytes<36>& uuid_string) -> Uuid&;
  auto deserialize(const Core::Static::Bytes<32>& nibble_string) -> Uuid&;
  auto serialize() const -> const Core::Static::Bytes<36>;

  static auto generate() -> const Uuid;

 private:
  Core::Static::Vector<Bits_64, 2> low_high = {};
};

}  // namespace Perimortem::System