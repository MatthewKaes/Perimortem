// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/dynamic/map.hpp"

#include "tetrodotoxin/isa/context.hpp"
#include "tetrodotoxin/isa/definition.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Library {

class Scope;

using TypeMaterializer =
    const Ttx::Type* (*)(Scope & scope,
                         Ttx::Lexical::Cursor& cursor,
                         const Tetrodotoxin::Isa::Definition& definition);

// Scope owns the names that become visible while the Library ISA executes. It
// checks the current file first, then falls back to imported types exposed by
// the surrounding ISA context.
class Scope {
 public:
  explicit Scope(
      Tetrodotoxin::Isa::Context& context,
      TypeMaterializer materializer = nullptr)
      : context(context), materializer(materializer) {}

  auto define(const Ttx::Type& type) -> Bool;
  auto declare_type(
      const Tetrodotoxin::Isa::Definition& definition,
      Count body_index,
      Count next_index) -> Bool;
  auto materialize_type(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Core::View::Bytes name) -> const Ttx::Type*;
  auto seek_after_type(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Core::View::Bytes name) const -> Bool;
  auto find_type(Perimortem::Core::View::Bytes name) const -> const Ttx::Type*;
  auto resolve_type(Ttx::Lexical::Cursor& cursor) -> const Ttx::Type*;
  auto resolve_type(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Core::View::Bytes root_name) -> const Ttx::Type*;

  constexpr auto get_context() const -> Tetrodotoxin::Isa::Context& {
    return context;
  }

 private:
  enum class DeclarationState : Bits_8 {
    Pending,
    Evaluating,
    Ready,
    Failed,
  };

  class Declaration {
   public:
    Tetrodotoxin::Isa::Definition definition;
    Count body_index = 0;
    Count next_index = 0;
    const Ttx::Type* type = nullptr;
    DeclarationState state = DeclarationState::Pending;
  };

  Tetrodotoxin::Isa::Context& context;
  TypeMaterializer materializer = nullptr;
  Perimortem::Memory::Dynamic::Map<
      Perimortem::Core::View::Bytes,
      Declaration>
      declarations;
};

}  // namespace Tetrodotoxin::Isa::Library
