// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/isa/definition.hpp"
#include "tetrodotoxin/isa/library/scope.hpp"

namespace Tetrodotoxin::Isa::Library {

// Foreign keeps the authored foreign body available as a Library type while the
// dedicated foreign sub-ISA is still being designed.
class Foreign {
 public:
  static auto evaluate(
      Scope& scope,
      Ttx::Lexical::Cursor& cursor,
      const Tetrodotoxin::Isa::Definition& definition) -> const Ttx::Type*;

  static constexpr auto get_name() -> Perimortem::Core::View::Bytes {
    return "foreign"_view;
  }

 private:
  static auto skip_scope(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Core::View::Bytes message) -> Bool;
};

}  // namespace Tetrodotoxin::Isa::Library
