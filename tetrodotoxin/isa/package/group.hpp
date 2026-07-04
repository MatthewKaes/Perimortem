// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/isa/context.hpp"
#include "tetrodotoxin/isa/package/export.hpp"

namespace Tetrodotoxin::Isa::Package {

// Group owns a package `group` export block.
//
// A group publishes nested exports under one `::` segment. It is package
// syntax, not a Type-shaped qualifier pretending to be a namespace.
class Group {
 public:
  Group() = default;
  explicit Group(Perimortem::Core::View::Vector<Export> exports)
      : exports(exports), valid(True) {}

  static auto evaluate(
      Tetrodotoxin::Isa::Context& context,
      Ttx::Lexical::Cursor& cursor) -> Group;

  constexpr auto get_exports() const -> Perimortem::Core::View::Vector<Export> {
    return exports;
  }
  constexpr auto is_valid() const -> Bool { return valid; }

 private:
  Perimortem::Core::View::Vector<Export> exports;
  Bool valid = False;
};

}  // namespace Tetrodotoxin::Isa::Package
