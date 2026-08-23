// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"

namespace Perimortem::System {

// A 128 bit UUID value with canonical hexadecimal serialization. The two word
// representation keeps comparisons and hashing independent from text format.
// Construction accepts either 32 hexadecimal digits or the 36 byte dashed
// spelling. Generation provides random version 4 identifiers and time ordered
// version 7 identifiers.
class Uuid {
  static constexpr auto ascii_to_nibble(U8 byte) -> U64 {
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

  constexpr Uuid(const Uuid& rhs) : high_low(rhs.high_low) {}

  // Reads undashed hexadecimal in network order, highest word first.
  explicit constexpr Uuid(const Core::Static::Bytes<32>& source) {
    if consteval {
      for (Count i = 0; i < 16; i++) {
        high_low[0] |= ascii_to_nibble(source[i]) << (60 - i * 4);
      }

      for (Count i = 0; i < 16; i++) {
        high_low[1] |= ascii_to_nibble(source[i + 16]) << (60 - i * 4);
      }
    } else {
      deserialize(source);
    }
  }

  // Stores the high word followed by the low word.
  explicit constexpr Uuid(U64 high, U64 low) {
    high_low[0] = high;
    high_low[1] = low;
  }

  explicit constexpr Uuid(const Core::Static::Bytes<36>& source) {
    if consteval {
      for (Count i = 0; i < 8; i++) {
        high_low[0] |= ascii_to_nibble(source[i]) << (60 - i * 4);
      }

      for (Count i = 0; i < 4; i++) {
        high_low[0] |= ascii_to_nibble(source[i + 9]) << (28 - i * 4);
      }

      for (Count i = 0; i < 4; i++) {
        high_low[0] |= ascii_to_nibble(source[i + 14]) << (12 - i * 4);
      }

      for (Count i = 0; i < 4; i++) {
        high_low[1] |= ascii_to_nibble(source[i + 19]) << (60 - i * 4);
      }

      for (Count i = 0; i < 12; i++) {
        high_low[1] |= ascii_to_nibble(source[i + 24]) << (44 - i * 4);
      }
    } else {
      deserialize(source);
    }
  }

  constexpr auto operator==(const Uuid& rhs) const -> Bool {
    return high_low[0] == rhs.high_low[0] && high_low[1] == rhs.high_low[1];
  }

  constexpr auto operator!=(const Uuid& rhs) const -> Bool {
    return high_low[0] != rhs.high_low[0] || high_low[1] != rhs.high_low[1];
  }

  constexpr auto operator<(const Uuid& rhs) const -> Bool {
    if (high_low[0] == rhs.high_low[0]) {
      return high_low[1] < rhs.high_low[1];
    }

    return high_low[0] < rhs.high_low[0];
  }

  constexpr auto get_value() const -> const Core::Static::Vector<U64, 2> {
    return high_low;
  }

  constexpr auto is_set() const -> Bool {
    return high_low[0] != 0 || high_low[1] != 0;
  }

  auto deserialize(const Core::Static::Bytes<36>& uuid_string) -> Uuid&;
  auto deserialize(const Core::Static::Bytes<32>& nibble_string) -> Uuid&;
  auto serialize() const -> const Core::Static::Bytes<36>;

  static auto generate_v4() -> Uuid;
  static auto generate_v7() -> Uuid;

 private:
  Core::Static::Vector<U64, 2> high_low = {};
};

}  // namespace Perimortem::System
