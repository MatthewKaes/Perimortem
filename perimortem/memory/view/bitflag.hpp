// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/standard_types.hpp"

namespace Perimortem::Memory::View {

template <typename flag_source>
class BitFlag {
 public:
  using storage_type = Bits_64;

  constexpr auto convert_flag(flag_source flag) const -> storage_type {
    if (flag == static_cast<flag_source>(-1))
      return 0;
    else {
      return static_cast<storage_type>(1) << static_cast<storage_type>(flag);
    }
  }

  constexpr auto operator|(const BitFlag<flag_source>& lhs) const
      -> BitFlag<flag_source> {
    return BitFlag<flag_source>(bit_data | lhs.bit_data);
  }

  constexpr auto operator|(flag_source flag) const -> BitFlag<flag_source> {
    return BitFlag<flag_source>(bit_data | convert_flag(flag));
  }

  constexpr auto operator&(const BitFlag<flag_source>& lhs) const
      -> BitFlag<flag_source> {
    return BitFlag<flag_source>(
        static_cast<flag_source>(bit_data & lhs.bit_data));
  }

  constexpr auto operator^(const BitFlag<flag_source>& lhs) const
      -> BitFlag<flag_source> {
    return BitFlag<flag_source>(
        static_cast<flag_source>(bit_data ^ lhs.bit_data));
  }

  constexpr auto operator==(const BitFlag<flag_source>& flag) const -> bool {
    return bit_data == flag.bit_data;
  };

  constexpr auto operator!=(const BitFlag<flag_source>& flag) const -> bool {
    return bit_data != flag.bit_data;
  };

  constexpr auto has(flag_source flag) const -> bool {
    return bit_data & convert_flag(flag);
  };

  // Checks if the flag contains any of the flags in the target.
  constexpr auto any(const BitFlag<flag_source>& flag) const -> bool {
    return bit_data & flag.bit_data;
  };

  constexpr auto operator=(const BitFlag<flag_source>& flag)
      -> BitFlag<flag_source> {
    bit_data = flag.bit_data;
    return *this;
  };

  constexpr auto operator|=(const BitFlag<flag_source>& flag)
      -> BitFlag<flag_source> {
    bit_data |= flag.bit_data;
    return *this;
  };

  constexpr auto operator&=(const BitFlag<flag_source>& flag)
      -> BitFlag<flag_source> {
    bit_data &= flag.bit_data;
    return *this;
  };

  // Adds all flags to the set by ORing them in.
  constexpr auto operator+=(const BitFlag<flag_source>& flag)
      -> BitFlag<flag_source> {
    bit_data |= flag.bit_data;
    return *this;
  };

  // Creates a bitmask filtering out any of the included flags.
  constexpr auto operator-=(const BitFlag<flag_source>& flag)
      -> BitFlag<flag_source> {
    bit_data &= ~flag.bit_data;
    return *this;
  };

  auto clear() -> void { bit_data = 0; }
  auto raw() const -> storage_type { return bit_data; }

  BitFlag() : bit_data(0) {};
  BitFlag(storage_type data) : bit_data(data) {};
  BitFlag(flag_source data) : bit_data(convert_flag(data)) {};

 private:
  storage_type bit_data;
};

}  // namespace Perimortem::Memory::View

//
// Globally overloaded | operator for supporting types.
//
template <typename flag_source>
inline Perimortem::Memory::View::BitFlag<flag_source> operator|(
    flag_source lhs,
    flag_source rhs) {
  return Perimortem::Memory::View::BitFlag<flag_source>(lhs) |
         Perimortem::Memory::View::BitFlag<flag_source>(rhs);
}

//
// Globally overloaded & operator for supporting types.
//
template <typename flag_source>
inline Perimortem::Memory::View::BitFlag<flag_source> operator&(
    flag_source lhs,
    flag_source rhs) {
  return Perimortem::Memory::View::BitFlag<flag_source>(lhs) &
         Perimortem::Memory::View::BitFlag<flag_source>(rhs);
}
