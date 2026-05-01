// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/amorphous.hpp"

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
struct SourceInfo {
 private:
  struct AbiConverter {
   private:
    static constexpr auto extract_size(const char* src) -> Count {
      Count len = 0;
      while (src[len] != '\0')
        len++;
      return len;
    }

   public:
    AbiConverter(const std::source_location::__impl* data) {
      // Unable to get source information
      if (data == nullptr) {
        file = "<Unknown>"_view;
        function = "<Unknown>"_view;
        return;
      }

      file = Core::View::Amorphous(
          reinterpret_cast<const Byte*>(data->_M_file_name),
          extract_size(data->_M_file_name));
      function = Core::View::Amorphous(
          reinterpret_cast<const Byte*>(data->_M_function_name),
          extract_size(data->_M_function_name));

      line = data->_M_line;
      column = data->_M_column;
    }

    Core::View::Amorphous file = {};
    Core::View::Amorphous function = {};
    Count line = 0;
    Count column = 0;
  };

 public:
  // The magic that invokes the source location extension.
  static consteval auto current(
      const AbiConverter loc = AbiConverter(__builtin_source_location()))
      -> SourceInfo {
    return SourceInfo(loc);
  }

  constexpr SourceInfo(const AbiConverter abi) : abi(abi) {}

  constexpr auto get_line() const -> Count { return abi.line; }
  constexpr auto get_column() const -> Count { return abi.column; }
  constexpr auto get_file() const -> Core::View::Amorphous { return abi.file; }
  constexpr auto get_function() const -> Core::View::Amorphous {
    return abi.function;
  }

 private:
  const AbiConverter abi;
};

}  // namespace Perimortem::Core::Diagnostics