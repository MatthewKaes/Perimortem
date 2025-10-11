// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include <cstdint>
#include <type_traits>
#include <utility>

// Example usage:
//
// // File optoins
// enum class StorageOptions : uint8_t {
//   Preload,
//   Stream,
//   Compress,
//   _PERIMORTEM_ENABLE_BITFLAG(StorageOptions)
// };
//
// use StorageOptions::None for empty flag set.

namespace Perimortem::Concepts {

template <typename T, typename... U>
concept is_any_of = (std::same_as<T, U> || ...);

template <class T>
concept flag_storage_supported =
    is_any_of<T, uint8_t, uint16_t, uint32_t, uint64_t>;

template <class T, typename S>
concept marked_byte_flag =
    std::conditional_t<static_cast<S>(T::_ENABLE_BITFLAG) ==
                           static_cast<S>(-37),
                       std::true_type,
                       std::false_type>::type::value &&
    std::conditional_t<static_cast<S>(T::None) == static_cast<S>(-1),
                       std::true_type,
                       std::false_type>::type::value &&
    std::conditional_t<static_cast<S>(T::_MAX_BITFLAG) <= sizeof(S) * 8,
                       std::true_type,
                       std::false_type>::type::value;

template <typename flag_source, typename storage_type>
  requires std::is_scoped_enum_v<flag_source> &&
           flag_storage_supported<storage_type> &&
           marked_byte_flag<flag_source, storage_type>
class BitFlag {
 public:
  static constexpr uint8_t storage_size = sizeof(storage_type);

  constexpr auto convert_flag(flag_source flag) const -> storage_type {
    if (flag == static_cast<flag_source>(-1))
      return 0;
    else {
      return static_cast<storage_type>(1) << static_cast<storage_type>(flag);
    }
  }

  constexpr auto operator|(const BitFlag<flag_source, storage_type>& lhs) const
      -> BitFlag<flag_source, storage_type> {
    return BitFlag<flag_source, storage_type>(bit_data | lhs.bit_data);
  }

  constexpr auto operator|(flag_source flag) const
      -> BitFlag<flag_source, storage_type> {
    return BitFlag<flag_source, storage_type>(bit_data | convert_flag(flag));
  }

  constexpr auto operator&(const BitFlag<flag_source, storage_type>& lhs) const
      -> BitFlag<flag_source, storage_type> {
    return BitFlag<flag_source, storage_type>(
        static_cast<flag_source>(bit_data & lhs.bit_data));
  }

  constexpr auto operator^(const BitFlag<flag_source, storage_type>& lhs) const
      -> BitFlag<flag_source, storage_type> {
    return BitFlag<flag_source, storage_type>(
        static_cast<flag_source>(bit_data ^ lhs.bit_data));
  }

  constexpr auto operator==(
      const BitFlag<flag_source, storage_type>& flag) const -> bool {
    return bit_data == flag.bit_data;
  };

  constexpr auto operator!=(
      const BitFlag<flag_source, storage_type>& flag) const -> bool {
    return bit_data != flag.bit_data;
  };

  constexpr auto has(flag_source flag) const -> bool {
    return bit_data & convert_flag(flag);
  };

  // Checks if the flag contains any of the flags in the target.
  constexpr auto any(const BitFlag<flag_source, storage_type>& flag) const
      -> bool {
    return bit_data & flag.bit_data;
  };

  constexpr auto operator=(const BitFlag<flag_source, storage_type>& flag)
      -> BitFlag<flag_source, storage_type> {
    bit_data = flag.bit_data;
    return *this;
  };

  constexpr auto operator|=(const BitFlag<flag_source, storage_type>& flag)
      -> BitFlag<flag_source, storage_type> {
    bit_data |= flag.bit_data;
    return *this;
  };

  constexpr auto operator&=(const BitFlag<flag_source, storage_type>& flag)
      -> BitFlag<flag_source, storage_type> {
    bit_data &= flag.bit_data;
    return *this;
  };

  // Adds all flags to the set by ORing them in.
  constexpr auto operator+=(const BitFlag<flag_source, storage_type>& flag)
      -> BitFlag<flag_source, storage_type> {
    bit_data |= flag.bit_data;
    return *this;
  };

  // Creates a bitmask filtering out any of the included flags.
  constexpr auto operator-=(const BitFlag<flag_source, storage_type>& flag)
      -> BitFlag<flag_source, storage_type> {
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

}  // namespace Perimortem::Concepts

#define _PERIMORTEM_ENABLE_BITFLAG(e, storage)                                \
  _MAX_BITFLAG, None = static_cast<std::underlying_type_t<e>>(-1),            \
                _ENABLE_BITFLAG = static_cast<std::underlying_type_t<e>>(-37) \
  }                                                                           \
  ;                                                                           \
  static_assert(Perimortem::Concepts::flag_storage_supported<storage>,        \
                "Not the correct storage type. Valid types=uint8_t, "         \
                "uint16_t, uint32_t, uint64_t");                              \
  static_assert(                                                              \
      static_cast<size_t>(e::_MAX_BITFLAG) <= sizeof(storage) * 8,            \
      "Bit flag <enum class" #e                                               \
      "> contains more flags than it does bits! Use a larger storage size."); \
  using e##Flags = Perimortem::Concepts::BitFlag<e, storage>;                 \
  inline Perimortem::Concepts::BitFlag<e, storage> operator|(e lhs, e rhs) {  \
    return Perimortem::Concepts::BitFlag<e, storage>(lhs) |                   \
           Perimortem::Concepts::BitFlag<e, storage>(rhs);                    \
  }                                                                           \
  inline Perimortem::Concepts::BitFlag<e, storage> operator&(e lhs, e rhs) {  \
    return Perimortem::Concepts::BitFlag<e, storage>(lhs) &                   \
           Perimortem::Concepts::BitFlag<e, storage>(rhs);                    \
  }                                                                           \
  namespace {  // Empty namespace to capture the closing }