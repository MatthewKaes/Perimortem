// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "ttx/dialect/package/definition.hpp"
#include "ttx/dialect/symbol_path.hpp"

namespace Ttx::Dialect::Package {

// Export is one package-owned public declaration.
//
// It preserves the authored declaration so a later package binding pass can
// resolve aliases and package forwards against the already loaded imports.
// The resolved semantic result must still be a real Ttx::Type object published
// on Package, not a second package-specific type tree.
class Export {
 public:
  Export() = default;

  static auto parse(
      Lexical::Cursor& cursor,
      Ttx::Documentation documentation) -> Export;

  constexpr auto get_definition() const -> const Definition& {
    return definition;
  }
  constexpr auto get_target() const -> SymbolPath { return target; }
  constexpr auto get_exports() const
      -> Perimortem::Core::View::Vector<Export> {
    return exports;
  }
  constexpr auto is_valid() const -> Bool { return definition.is_valid(); }

 private:
  Export(Definition definition, SymbolPath target)
      : definition(definition), target(target) {}
  Export(
      Definition definition,
      Perimortem::Core::View::Vector<Export> exports)
      : definition(definition), exports(exports) {}

  Definition definition;
  SymbolPath target;
  Perimortem::Core::View::Vector<Export> exports;
};

}  // namespace Ttx::Dialect::Package
