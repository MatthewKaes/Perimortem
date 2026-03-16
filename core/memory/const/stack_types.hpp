// Perimortem Engine
// Copyright © Matt Kaes

//
// Perimortem Stack Types
//
/*
  Specialized types used for stack operations at compile time.

  The objects are a mix of generalized and extremely specialized and they should
  never be used outside of higher order Perimortem::Memory::Const unless you know
  what you are doing. :)
*/

#pragma once 

#include "core/memory/standard_types.hpp"

namespace Perimortem::Memory::Const {

// Constexpr method for static length of null terminated strings.
constexpr Count static_strlen(const char* str) {
  Count length = 0;
  while (str[length] != '\0')
    length++;

  return length;
}

// Constexpr method for getting the size of a compile time array.
template <typename T, Count N>
constexpr Count array_size(T (&)[N]) {
  return N;
}

// A fixed array string on the stack. It serves as a hybrid between std::array
// and c_strings. It has no null terminator but will pad with zeros.
template <uint64_t element_count>
class StackString {
 public:
  static const constexpr uint64_t storage_size = element_count;
  constexpr StackString(char const* data_) noexcept {
    source = data_;
    for (Int i = 0; i < static_strlen(data_); i++) {
      content[i] = data_[i];
    }
    length = static_strlen(data_);
  };

  constexpr StackString() noexcept {}

  constexpr auto data() const -> const char* const { return source; };
  constexpr auto size() const -> Int { return length; };

 private:
  char const* source;
  char content[element_count] = {};
  Int length = 0;
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
  return TablePair{key, value};
}

}  // namespace Perimortem::Memory::Const
