// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/utility/range.hpp"

#include "tetrodotoxin/isa/base/context.hpp"
#include "tetrodotoxin/isa/base/declaration.hpp"
#include "tetrodotoxin/isa/library/scope.hpp"
#include "tetrodotoxin/isa/lowering/context.hpp"
#include "tetrodotoxin/isa/lowering/input.hpp"
#include "ttx/documentation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa::Library {

// Library is the baseline body ISA for general TTX source files.
//
// Library constructs aliases, aggregate layout, foreign signatures, and
// function signatures as Ttx::Type facts. Function bodies continue
// directly into immutable compiler execution data rather than a Library AST.
class VirtualMachine {
 public:
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Isa::Base::Context& context) -> Ttx::Type*;
  static auto lower(
      Tetrodotoxin::Isa::Lowering::Context& context,
      const Tetrodotoxin::Isa::Lowering::Input& input) -> Bool;

  static constexpr auto get_name() -> Perimortem::Core::View::Bytes {
    return "Library"_view;
  }

 private:
  static auto lower_functions(
      Tetrodotoxin::Isa::Lowering::Context& context,
      const Tetrodotoxin::Isa::Lowering::Input& input,
      Perimortem::Core::View::Vector<Ttx::Function> functions) -> Bool;
  static auto lower_type(
      Tetrodotoxin::Isa::Lowering::Context& context,
      const Tetrodotoxin::Isa::Lowering::Input& input,
      const Ttx::Type& type) -> Bool;
  static auto evaluate_definition(
      Ttx::Lexical::Cursor& cursor,
      Library::Scope& scope,
      Ttx::Documentation documentation,
      Perimortem::Core::View::Vector<Ttx::Attribute> attributes,
      Perimortem::Memory::Managed::Vector<Ttx::Member>& members,
      Perimortem::Memory::Managed::Vector<Tetrodotoxin::Isa::Base::Definition>&
          member_definitions,
      Perimortem::Memory::Managed::Vector<Ttx::Type::Reference>& types,
      Perimortem::Memory::Managed::Vector<Ttx::Function>& functions,
      Perimortem::Memory::Managed::Vector<Perimortem::Utility::Range>&
          function_sources,
      Perimortem::Memory::Managed::Vector<Tetrodotoxin::Isa::Base::Definition>&
          function_definitions) -> Bool;
};

}  // namespace Tetrodotoxin::Isa::Library
