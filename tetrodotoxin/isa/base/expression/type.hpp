// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/isa/base/context.hpp"
#include "tetrodotoxin/isa/base/expression/evaluator.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Base::Expression {

class Type {
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

}  // namespace Tetrodotoxin::Isa::Base::Expression
