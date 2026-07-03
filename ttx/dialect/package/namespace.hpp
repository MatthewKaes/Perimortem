// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "ttx/dialect/package/export.hpp"

namespace Ttx::Dialect::Package {

// Namespace is the package subdialect for grouping public exports under a
// `::` segment. It owns only the authored export body; the semantic namespace
// is later materialized as a Ttx::Type with nested types when exports can bind.
class Namespace {
 public:
  Namespace() = default;
  explicit Namespace(Perimortem::Core::View::Vector<Export> exports)
      : exports(exports), valid(True) {}

  static auto parse(Lexical::Cursor& cursor) -> Namespace;

  constexpr auto get_exports() const
      -> Perimortem::Core::View::Vector<Export> {
    return exports;
  }
  constexpr auto is_valid() const -> Bool { return valid; }

 private:
  Perimortem::Core::View::Vector<Export> exports;
  Bool valid = False;
};

}  // namespace Ttx::Dialect::Package
