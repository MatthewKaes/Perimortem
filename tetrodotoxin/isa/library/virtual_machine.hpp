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
// Library constructs aliases, aggregate layout, foreign signatures, and
// callable function bodies as Ttx::Type facts. Function bodies stay as token
// blocks so compiler lowering can walk them without Library growing a second
// expression IR.
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
      Perimortem::Memory::Managed::Vector<const Ttx::Type*>& types,
      Perimortem::Memory::Managed::Vector<Ttx::Type::Function>& functions)
      -> Bool;
};

}  // namespace Tetrodotoxin::Isa::Library
