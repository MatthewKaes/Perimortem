// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/dynamic/map.hpp"

#include "perimortem/utility/range.hpp"

#include "tetrodotoxin/isa/base/context.hpp"
#include "tetrodotoxin/isa/base/declaration.hpp"
#include "tetrodotoxin/isa/library/staged_declaration.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Library {

class Scope;

using TypeMaterializer =
    const Ttx::Type* (*)(Ttx::Lexical::Cursor & cursor,
                         Scope& scope,
                         const Tetrodotoxin::Isa::Base::Declaration&
                             definition);

// Scope owns the names that become visible while the Library ISA executes. It
// checks the current file first, then falls back to imported types exposed by
// the surrounding ISA context.
class Scope {
 public:
  explicit Scope(
      Tetrodotoxin::Isa::Base::Context& context,
      TypeMaterializer materializer = nullptr)
      : context(context), materializer(materializer) {}

  auto define(const Ttx::Type& type) -> Bool;
  auto declare_type(
      const Tetrodotoxin::Isa::Base::Declaration& definition,
      Perimortem::Utility::Range source) -> Bool;
  auto materialize_type(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Core::View::Bytes name) -> const Ttx::Type*;
  auto stage_type_reference(
      Perimortem::Core::View::Bytes name,
      const Ttx::Type& type) -> Bool;
  auto seek_after_type(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Core::View::Bytes name) const -> Bool;
  auto find_type(Perimortem::Core::View::Bytes name) const -> const Ttx::Type*;
  auto resolve_type(Ttx::Lexical::Cursor& cursor) -> const Ttx::Type*;
  auto resolve_type(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Core::View::Bytes root_name) -> const Ttx::Type*;

  constexpr auto get_context() const -> Tetrodotoxin::Isa::Base::Context& {
    return context;
  }

 private:
  Tetrodotoxin::Isa::Base::Context& context;
  TypeMaterializer materializer = nullptr;
  Perimortem::Memory::Dynamic::
      Map<Perimortem::Core::View::Bytes, StagedDeclaration>
          declarations;
};

}  // namespace Tetrodotoxin::Isa::Library
