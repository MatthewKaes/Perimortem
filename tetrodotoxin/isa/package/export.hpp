// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/isa/package/definition.hpp"
#include "tetrodotoxin/isa/qualified_name.hpp"

namespace Tetrodotoxin::Isa {

// Export is one package-owned public declaration.
//
// It preserves the authored declaration so a later package binding pass can
// resolve aliases and package forwards against the already loaded imports.
// The resolved semantic result must still be a real Ttx::Type object published
// on Package, not a second package-specific type tree.
class Export {
 public:
  Export() = default;

  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Ttx::Documentation documentation) -> Export;

  constexpr auto get_definition() const -> const Definition& {
    return definition;
  }
  constexpr auto get_target() const -> QualifiedName { return target; }
  constexpr auto get_exports() const
      -> Perimortem::Core::View::Vector<Export> {
    return exports;
  }
  constexpr auto is_valid() const -> Bool { return definition.is_valid(); }

 private:
  Export(Definition definition, QualifiedName target)
      : definition(definition), target(target) {}
  Export(
      Definition definition,
      Perimortem::Core::View::Vector<Export> exports)
      : definition(definition), exports(exports) {}

  Definition definition;
  QualifiedName target;
  Perimortem::Core::View::Vector<Export> exports;
};

}  // namespace Tetrodotoxin::Isa
