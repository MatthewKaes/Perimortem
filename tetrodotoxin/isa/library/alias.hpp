// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/isa/definition.hpp"
#include "tetrodotoxin/isa/library/scope.hpp"

namespace Tetrodotoxin::Isa::Library {

// Alias turns a Library alias definition into a real Ttx::Type alias.
class Alias {
 public:
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Scope& scope,
      const Tetrodotoxin::Isa::Definition& definition) -> const Ttx::Type*;

  static constexpr auto get_name() -> Perimortem::Core::View::Bytes {
    return "alias"_view;
  }
};

}  // namespace Tetrodotoxin::Isa::Library
