// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/isa/definition.hpp"
#include "tetrodotoxin/isa/library/scope.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Library {

// Foreign owns Library ABI scopes. A function hosted by a foreign type is a
// bodyless declaration; the foreign type supplies the external lowering context.
class Foreign {
 public:
  static auto evaluate(
      Scope& scope,
      Ttx::Lexical::Cursor& cursor,
      const Tetrodotoxin::Isa::Definition& definition) -> const Ttx::Type*;

  static constexpr auto get_name() -> Perimortem::Core::View::Bytes {
    return "foreign"_view;
  }
};

}  // namespace Tetrodotoxin::Isa::Library
