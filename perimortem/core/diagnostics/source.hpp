// Perimortem Engine
// Copyright © Matt Kaes

// Should only be included in cpp files
// #pragma once

#include "perimortem/core/view/bytes.hpp"

// Compiler extension ABI
namespace std {
class source_location {
 public:
  struct __impl {
    // Null terminated cstring
    const char* _M_file_name;
    // Null terminated cstring
    const char* _M_function_name;
    unsigned _M_line;
    unsigned _M_column;
  };
};
}  // namespace std

namespace Perimortem::Core::Diagnostics {

// A class that wraps the compiler extensions to provide runtime
// access to source information.
struct Source {
 private:
  struct AbiConverter {
   private:
    static consteval auto extract_size(const char* src) -> Count {
      Count len = 0;
      while (src[len] != '\0') {
        len++;
      }
      return len;
    }

   public:
    consteval AbiConverter(const std::source_location::__impl* data) {
      // Unable to get source information
      if (data == nullptr) {
        return;
      }

      // Store the raw char* from the compiler ABI since we can't convert to
      // Byte* in a reasonable fashion that is actually constexpr.
      //
      // reinterpret_cast to Byte* is banned and we can't create a static vector
      // as the size isn't known until construction which is an annoying choice
      // for the C++ compiler API.
      //
      // The getters can convert to the proper type since we know it's valid and
      // for TTX we can just emit the correct data directly since we control the
      // compiler ABI.
      file = data->_M_file_name;
      file_size = extract_size(data->_M_file_name);
      func = data->_M_function_name;
      func_size = extract_size(data->_M_function_name);
      line = data->_M_line;
      column = data->_M_column;
    }

    const char* file = nullptr;
    Count file_size = 0;
    const char* func = nullptr;
    Count func_size = 0;
    Count line = 0;
    Count column = 0;
  };

 public:
  // The magic that invokes the source location extension.
  // For TTX we can make it so requests are embedded in the binary as read-only
  // data so the ABI only requires a pointer to the object in the binary.
  static consteval auto current(
      const AbiConverter loc = AbiConverter(__builtin_source_location()))
      -> Source {
    return Source(loc);
  }

  constexpr Source(const AbiConverter abi) : abi(abi) {}

  auto get_line() const -> Count;
  auto get_column() const -> Count;
  auto get_file() const -> Core::View::Bytes;
  auto get_function() const -> Core::View::Bytes;

 private:
  const AbiConverter abi;
};

}  // namespace Perimortem::Core::Diagnostics
