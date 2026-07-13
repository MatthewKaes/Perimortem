// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/isa/base/context.hpp"
#include "tetrodotoxin/isa/base/declaration.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Package {

// Export is one package-owned public declaration.
//
// It preserves the authored declaration and the result of executing its target
// type query. The resolved semantic result must still be a real Ttx::Type
// object published on Package, not a second package-specific type tree.
class Export {
 public:
  Export() = default;

  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Isa::Base::Context& context,
      Ttx::Documentation documentation) -> Export;

  static constexpr auto contains_name(
      Perimortem::Core::View::Vector<Export> exports,
      Perimortem::Core::View::Bytes name) -> Bool {
    for (Count i = 0; i < exports.get_size(); i++) {
      if (exports[i].get_definition().get_name() == name) {
        return True;
      }
    }

    return False;
  }

  constexpr auto get_definition() const
      -> const Tetrodotoxin::Isa::Base::Declaration& {
    return definition;
  }

  constexpr auto get_target() const -> const Ttx::Type* { return target; }
  constexpr auto get_exports() const -> Perimortem::Core::View::Vector<Export> {
    return exports;
  }

  constexpr auto is_valid() const -> Bool { return definition.is_valid(); }

 private:
  Export(
      Tetrodotoxin::Isa::Base::Declaration definition,
      const Ttx::Type* target)
      : definition(definition), target(target) {}
  Export(
      Tetrodotoxin::Isa::Base::Declaration definition,
      Perimortem::Core::View::Vector<Export> exports)
      : definition(definition), exports(exports) {}

  Tetrodotoxin::Isa::Base::Declaration definition;
  const Ttx::Type* target = nullptr;
  Perimortem::Core::View::Vector<Export> exports;
};

}  // namespace Tetrodotoxin::Isa::Package
