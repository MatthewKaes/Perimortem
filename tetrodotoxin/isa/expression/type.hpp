// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/isa/context.hpp"
#include "tetrodotoxin/isa/expression.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa {

class Expression::Type {
 public:
  static auto evaluate(Ttx::Lexical::Cursor& cursor, Context& context)
      -> const Ttx::Type*;
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Context& context,
      Perimortem::Core::View::Bytes root_name) -> const Ttx::Type*;
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Context& context,
      const Ttx::Type* root) -> const Ttx::Type*;
};

}  // namespace Tetrodotoxin::Isa
