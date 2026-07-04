// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/isa/context.hpp"
#include "tetrodotoxin/isa/package/export.hpp"

namespace Tetrodotoxin::Isa::Package {

// Namespace groups package exports under one `::` segment.
//
// It owns the authored export body for now. When exports can bind, Package
// should publish this as a real Ttx::Type with nested types rather than a
// parallel namespace tree.
class Namespace {
 public:
  Namespace() = default;
  explicit Namespace(Perimortem::Core::View::Vector<Export> exports)
      : exports(exports), valid(True) {}

  static auto evaluate(
      Tetrodotoxin::Isa::Context& context,
      Ttx::Lexical::Cursor& cursor) -> Namespace;

  constexpr auto get_exports() const -> Perimortem::Core::View::Vector<Export> {
    return exports;
  }
  constexpr auto is_valid() const -> Bool { return valid; }

 private:
  Perimortem::Core::View::Vector<Export> exports;
  Bool valid = False;
};

}  // namespace Tetrodotoxin::Isa::Package
