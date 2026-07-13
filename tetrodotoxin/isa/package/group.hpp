// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/base/context.hpp"
#include "tetrodotoxin/isa/package/export.hpp"

namespace Tetrodotoxin::Isa::Package {

// Parses a package `group` export block into the caller's dense export table.
//
// A group publishes nested exports under one `::` segment. It is package
// syntax, not a persistent object or a Type-shaped qualifier pretending to be
// a namespace.
class Group {
 public:
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Isa::Base::Context& context,
      Perimortem::Memory::Managed::Vector<Export>& exports) -> Bool;
};

}  // namespace Tetrodotoxin::Isa::Package
