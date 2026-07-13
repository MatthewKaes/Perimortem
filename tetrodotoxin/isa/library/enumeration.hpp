// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/isa/base/declaration.hpp"
#include "tetrodotoxin/isa/library/scope.hpp"

namespace Tetrodotoxin::Isa::Library {

// Enumeration owns brace-scoped Library `enum[Storage]` declarations and
// presents enum cases as compile-time members of the carrier storage type.
class Enumeration {
 public:
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Scope& scope,
      const Tetrodotoxin::Isa::Base::Declaration& definition)
      -> const Ttx::Type*;

  static constexpr auto get_name() -> Perimortem::Core::View::Bytes {
    return "enum"_view;
  }
};

}  // namespace Tetrodotoxin::Isa::Library
