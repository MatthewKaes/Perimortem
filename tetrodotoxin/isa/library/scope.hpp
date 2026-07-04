// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/isa/context.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Library {

// Scope owns the names that become visible while the Library ISA executes. It
// checks the current file first, then falls back to imported types exposed by
// the surrounding ISA context.
class Scope {
 public:
  explicit Scope(Tetrodotoxin::Isa::Context& context) : context(context) {}

  auto define(const Ttx::Type& type) -> Bool;
  auto resolve_type(Ttx::Lexical::Cursor& cursor) const -> const Ttx::Type*;
  auto resolve_type(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Core::View::Bytes root_name) const -> const Ttx::Type*;

  constexpr auto get_context() const -> Tetrodotoxin::Isa::Context& {
    return context;
  }

 private:
  Tetrodotoxin::Isa::Context& context;
};

}  // namespace Tetrodotoxin::Isa::Library
