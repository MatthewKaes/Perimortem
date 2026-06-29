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

// Thin Perimortem shim over the compiler's source location ABI.
// Sorce stores only a pointer to the static __impl struct baked into the binary
// and uses value semantics so a Source copy is always a single pointer copy.
//
// TTX can provide source information by embedding the same information as the
// ABI making it useful for cross language diagnostics.
//
// Accessors are evaluated lazily since evaluating source most likely means we
// are already in a diagnostics slow path.
struct Source {
 public:
  // Captures the call site via the standard function-default-param pattern.
  // The __impl pointer points to read-only data in the binary.
  static consteval auto current(
      const std::source_location::__impl* impl = __builtin_source_location())
      -> Source {
    return Source(impl);
  }

  constexpr Source() = default;
  constexpr explicit Source(const std::source_location::__impl* impl)
      : impl(impl) {}
  constexpr Source(
      Core::View::Bytes file,
      Count line,
      Count column,
      Core::View::Bytes function = Core::View::Bytes())
      : explicit_file(file),
        explicit_function(function),
        explicit_line(line),
        explicit_column(column) {}

  auto is_set() const -> Bool;
  auto get_line() const -> Count;
  auto get_column() const -> Count;
  auto get_file() const -> Core::View::Bytes;
  auto get_function() const -> Core::View::Bytes;

 private:
  const std::source_location::__impl* impl = nullptr;
  Core::View::Bytes explicit_file;
  Core::View::Bytes explicit_function;
  Count explicit_line = 0;
  Count explicit_column = 0;
};

}  // namespace Perimortem::Core::Diagnostics
