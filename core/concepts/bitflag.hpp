// Perimortem Engine
// Copyright © Matt Kaes

//
// A generalized Bit Flag concept that enables wrapping any type that support
// that supports bit like opperations and can be stored into a storage type.
/*
  The classic usage is for enum classes, however you can use a struct or class
  which implements taking in the type and converting it to a bit position in a
  unique and stable way.

  Example usage with enum Class:

  // File optoins
  enum class StorageOptions : uint8_t {
    None = -1,
    Preload,
    Stream,
    Compress,
    TOTAL_FLAGS,
  };

  Here you can use StorageOptions::None for empty flag set.
*/

#pragma once

#include <cstdint>
#include <type_traits>
#include <utility>

// Example usag with enum Class:
//
// // File optoins
// enum class StorageOptions : uint8_t {
//   None = -1,
//   Preload,
//   Stream,
//   Compress,
//   TOTAL_FLAGS,
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
    std::conditional_t<static_cast<S>(T::None) == static_cast<S>(-1),
                       std::true_type,
                       std::false_type>::type::value &&
    std::conditional_t<(static_cast<S>(T::TOTAL_FLAGS) <= sizeof(S) * 8),
                       std::true_type,
                       std::false_type>::type::value;

template <class T>
concept uses_uint8_t_stroage =
    std::conditional_t<static_cast<uint16_t>(T::None) == static_cast<uint16_t>(-1),
                       std::true_type,
                       std::false_type>::type::value &&
    std::conditional_t<(static_cast<uint16_t>(T::TOTAL_FLAGS) <= sizeof(uint8_t) * 8),
                       std::true_type,
                       std::false_type>::type::value;

template <class T>
concept uses_uint16_t_stroage =
    std::conditional_t<static_cast<uint16_t>(T::None) == static_cast<uint16_t>(-1),
                       std::true_type,
                       std::false_type>::type::value &&
    std::conditional_t<(static_cast<uint16_t>(T::TOTAL_FLAGS) > sizeof(uint8_t) * 8),
                       std::true_type,
                       std::false_type>::type::value &&
    std::conditional_t<(static_cast<uint16_t>(T::TOTAL_FLAGS) <= sizeof(uint16_t) * 8),
                       std::true_type,
                       std::false_type>::type::value;

template <class T>
concept uses_uint32_t_stroage =
    std::conditional_t<static_cast<uint16_t>(T::None) == static_cast<uint16_t>(-1),
                       std::true_type,
                       std::false_type>::type::value &&
    std::conditional_t<(static_cast<uint16_t>(T::TOTAL_FLAGS) > sizeof(uint16_t) * 8),
                       std::true_type,
                       std::false_type>::type::value &&
    std::conditional_t<(static_cast<uint16_t>(T::TOTAL_FLAGS) <= sizeof(uint32_t) * 8),
                       std::true_type,
                       std::false_type>::type::value;

template <class T>
concept uses_uint64_t_stroage =
    std::conditional_t<static_cast<uint16_t>(T::None) == static_cast<uint16_t>(-1),
                       std::true_type,
                       std::false_type>::type::value &&
    std::conditional_t<(static_cast<uint16_t>(T::TOTAL_FLAGS) > sizeof(uint32_t) * 8),
                       std::true_type,
                       std::false_type>::type::value &&
    std::conditional_t<(static_cast<uint16_t>(T::TOTAL_FLAGS) <= sizeof(uint64_t) * 8),
                       std::true_type,
                       std::false_type>::type::value;

template <typename flag_source, typename storage_type>
  requires flag_storage_supported<storage_type> &&
           marked_byte_flag<flag_source, storage_type>
class BitFlag {
  static_assert(Perimortem::Concepts::flag_storage_supported<storage_type>,
                "Not the correct storage type. Valid types=uint8_t, "
                "uint16_t, uint32_t, uint64_t");
  static_assert(
      static_cast<uint64_t>(flag_source::TOTAL_FLAGS) <= sizeof(storage_type) * 8,
      "Bit flag contains more flags than it does bits! Use a larger storage size.");

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

//
// Globally overloaded | operator for supporting types.
//
template <typename flag_source>
requires Perimortem::Concepts::uses_uint8_t_stroage<flag_source>
inline Perimortem::Concepts::BitFlag<flag_source, uint8_t> operator|(flag_source lhs, flag_source rhs) {
  return Perimortem::Concepts::BitFlag<flag_source, uint8_t>(lhs) |
         Perimortem::Concepts::BitFlag<flag_source, uint8_t>(rhs);
}

template <typename flag_source>
requires Perimortem::Concepts::uses_uint16_t_stroage<flag_source>
inline Perimortem::Concepts::BitFlag<flag_source, uint16_t> operator|(flag_source lhs, flag_source rhs) {
  return Perimortem::Concepts::BitFlag<flag_source, uint16_t>(lhs) |
         Perimortem::Concepts::BitFlag<flag_source, uint16_t>(rhs);
}

template <typename flag_source>
requires Perimortem::Concepts::uses_uint32_t_stroage<flag_source>
inline Perimortem::Concepts::BitFlag<flag_source, uint32_t> operator|(flag_source lhs, flag_source rhs) {
  return Perimortem::Concepts::BitFlag<flag_source, uint32_t>(lhs) |
         Perimortem::Concepts::BitFlag<flag_source, uint32_t>(rhs);
}

template <typename flag_source>
requires Perimortem::Concepts::uses_uint64_t_stroage<flag_source>
inline Perimortem::Concepts::BitFlag<flag_source, uint64_t> operator|(flag_source lhs, flag_source rhs) {
  return Perimortem::Concepts::BitFlag<flag_source, uint64_t>(lhs) |
         Perimortem::Concepts::BitFlag<flag_source, uint64_t>(rhs);
}

//
// Globally overloaded & operator for supporting types.
//
template <typename flag_source>
requires Perimortem::Concepts::uses_uint8_t_stroage<flag_source>
inline Perimortem::Concepts::BitFlag<flag_source, uint8_t> operator&(flag_source lhs, flag_source rhs) {
  return Perimortem::Concepts::BitFlag<flag_source, uint8_t>(lhs) &
         Perimortem::Concepts::BitFlag<flag_source, uint8_t>(rhs);
}

template <typename flag_source>
requires Perimortem::Concepts::uses_uint16_t_stroage<flag_source>
inline Perimortem::Concepts::BitFlag<flag_source, uint16_t> operator&(flag_source lhs, flag_source rhs) {
  return Perimortem::Concepts::BitFlag<flag_source, uint16_t>(lhs) &
         Perimortem::Concepts::BitFlag<flag_source, uint16_t>(rhs);
}

template <typename flag_source>
requires Perimortem::Concepts::uses_uint32_t_stroage<flag_source>
inline Perimortem::Concepts::BitFlag<flag_source, uint32_t> operator&(flag_source lhs, flag_source rhs) {
  return Perimortem::Concepts::BitFlag<flag_source, uint32_t>(lhs) &
         Perimortem::Concepts::BitFlag<flag_source, uint32_t>(rhs);
}

template <typename flag_source>
requires Perimortem::Concepts::uses_uint64_t_stroage<flag_source>
inline Perimortem::Concepts::BitFlag<flag_source, uint64_t> operator&(flag_source lhs, flag_source rhs) {
  return Perimortem::Concepts::BitFlag<flag_source, uint64_t>(lhs) &
         Perimortem::Concepts::BitFlag<flag_source, uint64_t>(rhs);
}
