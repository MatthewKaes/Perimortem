// Perimortem Engine
// Copyright © Matt Kaes

//
// Perimortem Stack Types
//
/*
  Specialized types used for stack operations at compile time.

  The objects are a mix of generalized and extremely specialized and they should
  never be used outside of higher order Perimortem::Concepts unless you know
  what you are doing. :)
*/

#pragma once 

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

namespace Perimortem::Concepts {
// A fixed array string on the stack. It serves as a hybrid between std::array
// and c_strings. It has no null terminator but will pad with zeros.
template <uint64_t element_count>
class StackString {
 public:
  static const constexpr uint64_t storage_size = element_count;
  constexpr StackString(char const* data_) noexcept {
    for (uint32_t i = 0; i < std::char_traits<char>::length(data_); i++) {
      content[i] = data_[i];
    }
    length = std::char_traits<char>::length(data_);
  };

  constexpr StackString() noexcept {}

  constexpr auto data() const -> const char* const { return content; };
  constexpr auto size() const -> uint32_t { return length; };

 private:
  char content[element_count] = {};
  uint32_t length = 0;
};

// Simpilest possible std::pair type for.
template <typename key_type, typename value_type>
struct TablePair {
  key_type key;
  value_type value;
};

// Helper for forcing table pairs to aid array narrowing.
template <typename value_type>
constexpr auto make_pair(const char* const key, value_type&& value)
    -> TablePair<const char*, value_type> {
  return TablePair{key, std::forward<value_type>(value)};
}

// Constexpr method for getting the size of a compile time array.
template <typename T, uint64_t N>
constexpr uint64_t array_size(T (&)[N]) {
  return N;
}

// Gets the largest power of 2 stride we can take without overshooting
// the bounds of an array. Only goes up to 8 as anything beyond has
// diminishing returns for Perimortem usecases.
inline static constexpr uint8_t radix_stride(uint64_t value) {
  constexpr const uint8_t max_bit = 4;
  constexpr const uint8_t test_count = max_bit - 1;

  uint8_t radix_count = 0;
  while (radix_count < test_count) {
    if ((value & (1 << radix_count)) != 0)
      break;

    radix_count++;
  }

  return 1 << radix_count;
}

// Simple test to ensure it's working but also as self documentation.
static_assert(radix_stride(0) == 8);
static_assert(radix_stride(1) == 1);
static_assert(radix_stride(2) == 2);
static_assert(radix_stride(3) == 1);
static_assert(radix_stride(4) == 4);
static_assert(radix_stride(5) == 1);
static_assert(radix_stride(6) == 2);
static_assert(radix_stride(7) == 1);
static_assert(radix_stride(8) == 8);
static_assert(radix_stride(9) == 1);

}  // namespace Perimortem::Concepts
