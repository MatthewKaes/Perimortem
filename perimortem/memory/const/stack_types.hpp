// Perimortem Engine
// Copyright © Matt Kaes

//
// Perimortem Stack Types
//
/*
  Specialized types used for stack operations at compile time.
*/

#pragma once 

#include "perimortem/memory/const/standard_types.hpp"

namespace Perimortem::Memory::Const {


// Constexpr method for getting the size of a compile time array.
template <typename T, Count N>
constexpr Count array_size(T (&)[N]) {
  return N;
}

// A fixed array string on the stack. It serves as a hybrid between std::array
// and c_strings. It has no null terminator but will pad with zeros.
template <Count element_count>
class StackString {
 public:
  static const constexpr Count storage_size = element_count;
  constexpr StackString(char const* data_) noexcept {
    for (Int i = 0; i < static_strlen(std::bit_cast<Byte const*>(data_)); i++) {
      content[i] = data_[i];
    }
    length = static_strlen(std::bit_cast<Byte const*>(data_));
    source = &content;
  };

  constexpr StackString() noexcept {}

  constexpr auto get_data() const -> const Byte* const { return source; };
  constexpr auto get_size() const -> Count { return length; };

 private:
  // Constexpr method for static length of null terminated strings.
  constexpr Count static_strlen(const char* str) {
    Count length = 0;
    while (str[length] != '\0')
      length++;

    return length;
  }
  
  Byte const* source;
  Count length = 0;
  Byte content[element_count] = {};
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
