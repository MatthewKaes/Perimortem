// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "ttx/parse/symbol_path.hpp"

namespace Ttx::Parse {

// Import is the unresolved dependency request written in the source envelope.
//
// Resolution owns loading and binding because only the package manager knows
// the source tree and the already loaded package graph. The parser preserves
// just the authored request. That request contains the local alias, the
// expected dialect name, and a source selector.
//
// A package selector such as `TTX::Graphics` names a package that resolution can
// map to a package file. A file selector such as
// `(.source = "library/types.ttx")` names a concrete source relative to the
// package currently being read. After resolution succeeds, TTX receives the
// imported types under the requested local names and the same cursor continues
// into the selected dialect body.
//
// File opening, package path normalization, and symbol binding stay in
// resolution so there is one owner for the source tree and package graph.
// Import stays the source-authored request that feeds that layer.
class Import {
 public:
  static auto parse(Cursor& cursor) -> Import;

  constexpr auto get_local_name() const -> Perimortem::Core::View::Bytes {
    return local_name;
  }
  constexpr auto get_dialect_name() const -> const SymbolPath& {
    return dialect_name;
  }
  constexpr auto get_package_path() const -> const SymbolPath& {
    return package_path;
  }
  constexpr auto get_file_path() const -> Perimortem::Core::View::Bytes {
    return file_path;
  }

  // The envelope parser fills exactly one source selector on a parsed import.
  // If both are empty, this object is the absent or malformed import sentinel.
  constexpr auto is_package_source() const -> Bool {
    return !package_path.is_empty();
  }
  constexpr auto is_file_source() const -> Bool { return !file_path.is_empty(); }

  // Empty imports are parse sentinels. A real import must have a local alias,
  // so Source can skip malformed imports without inventing a second status
  // object.
  constexpr auto is_empty() const -> Bool { return local_name.is_empty(); }

 private:
  static constexpr auto inner_text(Perimortem::Core::View::Bytes text)
      -> Perimortem::Core::View::Bytes;

  Perimortem::Core::View::Bytes local_name;
  SymbolPath dialect_name;
  SymbolPath package_path;
  Perimortem::Core::View::Bytes file_path;
};

}  // namespace Ttx::Parse
