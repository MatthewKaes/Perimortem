// Perimortem Engine
// Copyright © Matt Kaes

//
// Perimortem BitFlag
//
/*
  A generalized Bit Flag concept that enables wrapping any type that support
  that supports bit like opperations and can be stored into a storage type.

  The classic usage is for enum classes, however you can use a struct or class
  which implements taking in the type and converting it to a bit position in a
  unique and stable way.

  Example usage with enum Class:

  // File optoins
  enum class StorageOptions : Bits_8 {
    None = -1,
    Preload,
    Stream,
    Compress,
    TOTAL_FLAGS,
  };

  Here you can use StorageOptions::None for empty flag set.
*/

#pragma once

#include "perimortem/memory/const/standard_types.hpp"

#include <type_traits>
#include <utility>

// Example usag with enum Class:
//
// // File optoins
// enum class StorageOptions : Bits_8 {
//   None = -1,
//   Preload,
//   Stream,
//   Compress,
//   TOTAL_FLAGS,
// };
//
// use StorageOptions::None for empty flag set.

namespace Perimortem::Memory::View {

template <typename T, typename... U>
concept is_any_of = (std::same_as<T, U> || ...);

template <class T>
concept flag_storage_supported =
    is_any_of<T, Bits_8, Bits_16, Bits_32, Bits_64, Bits_128>;

template <class T>
concept marked_byte_flag =
    std::conditional_t<static_cast<Bits_8>(T::None) == static_cast<Bits_8>(-1),
                       std::true_type,
                       std::false_type>::type::value &&
    std::conditional_t<(static_cast<Bits_8>(T::TOTAL_FLAGS) > 0),
                       std::true_type,
                       std::false_type>::type::value;

template <class T>
concept uses_Bits_8_stroage =
    std::conditional_t<static_cast<Bits_8>(T::None) ==
                           static_cast<Bits_8>(-1),
                       std::true_type,
                       std::false_type>::type::value &&
    std::conditional_t<(static_cast<Bits_16>(T::TOTAL_FLAGS) <=
                        size_in_bits<Bits_8>()),
                       std::true_type,
                       std::false_type>::type::value;

template <class T>
concept uses_Bits_16_stroage =
    std::conditional_t<static_cast<Bits_16>(T::None) ==
                           static_cast<Bits_16>(-1),
                       std::true_type,
                       std::false_type>::type::value &&
    std::conditional_t<(static_cast<Bits_16>(T::TOTAL_FLAGS) >
                        size_in_bits<Bits_8>()),
                       std::true_type,
                       std::false_type>::type::value &&
    std::conditional_t<(static_cast<Bits_16>(T::TOTAL_FLAGS) <=
                        size_in_bits<Bits_16>()),
                       std::true_type,
                       std::false_type>::type::value;

template <class T>
concept uses_Bits_32_stroage =
    std::conditional_t<static_cast<Bits_32>(T::None) ==
                           static_cast<Bits_16>(-1),
                       std::true_type,
                       std::false_type>::type::value &&
    std::conditional_t<(static_cast<Bits_16>(T::TOTAL_FLAGS) >
                        size_in_bits<Bits_16>()),
                       std::true_type,
                       std::false_type>::type::value &&
    std::conditional_t<(static_cast<Bits_32>(T::TOTAL_FLAGS) <=
                        size_in_bits<Bits_32>()),
                       std::true_type,
                       std::false_type>::type::value;

template <class T>
concept uses_Bits_64_stroage =
    std::conditional_t<static_cast<Bits_16>(T::None) ==
                           static_cast<Bits_16>(-1),
                       std::true_type,
                       std::false_type>::type::value &&
    std::conditional_t<(static_cast<Bits_16>(T::TOTAL_FLAGS) >
                        size_in_bits<Bits_32>()),
                       std::true_type,
                       std::false_type>::type::value &&
    std::conditional_t<(static_cast<Bits_64>(T::TOTAL_FLAGS) <=
                        size_in_bits<Bits_64>()),
                       std::true_type,
                       std::false_type>::type::value;

template <class T>
concept uses_Bits_128_stroage =
    std::conditional_t<static_cast<Bits_16>(T::None) ==
                           static_cast<Bits_16>(-1),
                       std::true_type,
                       std::false_type>::type::value &&
    std::conditional_t<(static_cast<Bits_16>(T::TOTAL_FLAGS) >
                        size_in_bits<Bits_64>()),
                       std::true_type,
                       std::false_type>::type::value &&
    std::conditional_t<(static_cast<Bits_16>(T::TOTAL_FLAGS) <=
                        size_in_bits<Bits_128>()),
                       std::true_type,
                       std::false_type>::type::value;

template <class T>
requires marked_byte_flag<T>
constexpr auto storage_from_flag() {
  constexpr Bits_8 flag_count = static_cast<Bits_8>(T::TOTAL_FLAGS);
  if constexpr (flag_count <= size_in_bits<Bits_8>())
    return std::type_identity<Bits_8>{};
  else if constexpr (flag_count <= size_in_bits<Bits_16>())
    return std::type_identity<Bits_16>{};
  else if constexpr (flag_count <= size_in_bits<Bits_32>())
    return std::type_identity<Bits_32>{};
  else if constexpr (flag_count <= size_in_bits<Bits_64>())
    return std::type_identity<Bits_64>{};
  else if constexpr (flag_count <= size_in_bits<Bits_128>())
    return std::type_identity<Bits_128>{};
  else
    return std::type_identity<void>{};
}

template <typename flag_source>
class BitFlag {
 public:
  using storage_type =
      typename decltype(storage_from_flag<flag_source>())::type;
  static_assert(Perimortem::Memory::View::flag_storage_supported<storage_type>,
                "Not the correct storage type. Valid types=Bits_8, "
                "Bits_16, Bits_32, Bits_64");
  static_assert(static_cast<Bits_64>(flag_source::TOTAL_FLAGS) <=
                    sizeof(storage_type) * 8,
                "Bit flag contains more flags than it does bits! Use a larger "
                "storage size.");

  static constexpr Bits_8 storage_size = sizeof(storage_type);

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

  auto clear() -> void { bit_data = convert_flag(flag_source::None); }
  auto raw() const -> storage_type { return bit_data; }

  BitFlag() : bit_data(convert_flag(flag_source::None)) {};
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
  requires Perimortem::Memory::View::flag_storage_supported<
      typename decltype(Perimortem::Memory::View::storage_from_flag<
                        flag_source>())::type>
inline Perimortem::Memory::View::BitFlag<flag_source> operator|(flag_source lhs,
                                                            flag_source rhs) {
  return Perimortem::Memory::View::BitFlag<flag_source>(lhs) |
         Perimortem::Memory::View::BitFlag<flag_source>(rhs);
}

//
// Globally overloaded & operator for supporting types.
//
template <typename flag_source>
  requires Perimortem::Memory::View::flag_storage_supported<
      typename decltype(Perimortem::Memory::View::storage_from_flag<
                        flag_source>())::type>
inline Perimortem::Memory::View::BitFlag<flag_source> operator&(flag_source lhs,
                                                            flag_source rhs) {
  return Perimortem::Memory::View::BitFlag<flag_source>(lhs) &
         Perimortem::Memory::View::BitFlag<flag_source>(rhs);
}
