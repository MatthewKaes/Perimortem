// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/context.hpp"
#include "tetrodotoxin/isa/definition.hpp"
#include "tetrodotoxin/isa/library/scope.hpp"
#include "ttx/documentation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa::Library {

// Library is the baseline body ISA for general TTX source files.
//
// This first slice keeps the instruction set intentionally small: Library can
// publish aliases, structs, and shallow foreign scopes as real Ttx::Type facts.
// Values, expressions, and function bodies come later after the
// package/compiler spine can already ask useful `::` and `.` questions.
class VirtualMachine {
 public:
  static auto evaluate(
      Tetrodotoxin::Isa::Context& context,
      Ttx::Lexical::Cursor& cursor) -> Ttx::Type*;

  static constexpr auto get_name() -> Perimortem::Core::View::Bytes {
    return "Library"_view;
  }

 private:
  static auto evaluate_definition(
      Library::Scope& scope,
      Ttx::Lexical::Cursor& cursor,
      Ttx::Documentation documentation,
      Perimortem::Memory::Managed::Vector<Ttx::Type::Member>& members,
      Perimortem::Memory::Managed::Vector<const Ttx::Type*>& types) -> Bool;
};

}  // namespace Tetrodotoxin::Isa::Library
