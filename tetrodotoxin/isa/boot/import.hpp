// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/isa/registry.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa::Boot {

// Import is the unresolved dependency request written in the source envelope.
//
// Resolution owns loading and binding because only the package manager knows
// the source tree and the already loaded package graph. Boot preserves
// just the authored request. That request contains the local alias, the
// expected ISA name, and a source selector.
//
// A package selector such as `Perimortem::Graphics` names a package that
// resolution can map to a package file. A file selector string such as
// `"library/types.ttx"` names a concrete source relative to the
// package currently being read. After resolution succeeds, TTX receives the
// imported types under the requested local names and the declared ISA evaluates
// the remaining bytecode.
//
// Imports only support forward `/` as a separator regardless of backend. The
// actual file opening, package path normalization, and symbol binding stay in
// the `Resolution` layer which centralizes the OS related operations.
class Import {
 public:
  Import() = default;

  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      const Tetrodotoxin::Isa::Registry& registry) -> Import;

  constexpr auto get_local_name() const -> Perimortem::Core::View::Bytes {
    return local_name;
  }
  constexpr auto get_isa() const -> Perimortem::Core::View::Bytes {
    return isa;
  }
  constexpr auto get_source_name() const -> Perimortem::Core::View::Bytes {
    return source_name;
  }

  // True when the import names a package instead of a source file.
  constexpr auto is_package() const -> Bool { return package; }

  // True when the source envelope produced a complete import request.
  constexpr auto is_valid() const -> Bool {
    return !local_name.is_empty() && !source_name.is_empty() &&
           !isa.is_empty();
  }

 private:
  Import(
      Perimortem::Core::View::Bytes local_name,
      Perimortem::Core::View::Bytes source_name,
      Perimortem::Core::View::Bytes isa,
      Bool package)
      : local_name(local_name),
        source_name(source_name),
        isa(isa),
        package(package) {}

  Perimortem::Core::View::Bytes local_name;
  Perimortem::Core::View::Bytes source_name;
  Perimortem::Core::View::Bytes isa;
  Bool package = False;
};

}  // namespace Tetrodotoxin::Isa::Boot
