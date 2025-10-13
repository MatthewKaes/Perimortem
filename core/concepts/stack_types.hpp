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

}  // namespace Perimortem::Concepts
