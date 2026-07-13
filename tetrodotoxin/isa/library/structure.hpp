// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/isa/base/declaration.hpp"
#include "tetrodotoxin/isa/library/scope.hpp"

namespace Tetrodotoxin::Isa::Library {

// Structure owns Library `struct` bodies and constructs their member layout.
class Structure {
 public:
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Scope& scope,
      const Tetrodotoxin::Isa::Base::Declaration& definition)
      -> const Ttx::Type*;

  static constexpr auto get_name() -> Perimortem::Core::View::Bytes {
    return "struct"_view;
  }
};

}  // namespace Tetrodotoxin::Isa::Library
