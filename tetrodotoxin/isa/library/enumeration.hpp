// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/isa/definition.hpp"
#include "tetrodotoxin/isa/library/scope.hpp"

namespace Tetrodotoxin::Isa::Library {

// Enumeration owns Library `enum[Storage]` declarations and presents enum
// cases as compile-time members of the carrier storage type.
class Enumeration {
 public:
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Scope& scope,
      const Tetrodotoxin::Isa::Definition& definition) -> const Ttx::Type*;

  static constexpr auto get_name() -> Perimortem::Core::View::Bytes {
    return "enum"_view;
  }
};

}  // namespace Tetrodotoxin::Isa::Library
