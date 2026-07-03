// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "ttx/dialect/source/dialect.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Ttx::Dialect::Source {

// Import is the unresolved dependency request written in the source envelope.
//
// Resolution owns loading and binding because only the package manager knows
// the source tree and the already loaded package graph. The parser preserves
// just the authored request. That request contains the local alias, the
// expected dialect name, and a source selector.
//
// A package selector such as `TTX::Graphics` names a package that resolution
// can map to a package file. A file selector such as
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
  Import() = default;
  Import(
      Perimortem::Core::View::Bytes local_name,
      Perimortem::Core::View::Bytes source_name,
      Dialect dialect,
      Bool package)
      : local_name(local_name),
        source_name(source_name),
        dialect(dialect),
        package(package) {}
  static auto should_parse(Lexical::Cursor& cursor) -> Bool;
  static auto parse(Lexical::Cursor& cursor) -> Import;

  constexpr auto get_local_name() const -> Perimortem::Core::View::Bytes {
    return local_name;
  }
  constexpr auto get_dialect() const -> const Dialect& { return dialect; }
  constexpr auto get_source_name() const -> Perimortem::Core::View::Bytes {
    return source_name;
  }

  // Checks if the key should use package semantics or not.
  constexpr auto is_package() const -> Bool { return package; }

  // Checks if the key should use package semantics or not.
  constexpr auto is_valid() const -> Bool {
    return !local_name.is_empty() && !source_name.is_empty() &&
           dialect.is_valid();
  }

 private:
  Perimortem::Core::View::Bytes local_name;
  Perimortem::Core::View::Bytes source_name;
  Dialect dialect;
  Bool package = False;
};

}  // namespace Ttx::Dialect::Source
