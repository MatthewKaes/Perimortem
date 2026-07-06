// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/isa/context.hpp"
#include "ttx/documentation.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Shader {

// Function owns shader stage syntax.
class Function {
 public:
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Isa::Context& context,
      Ttx::Documentation documentation) -> Ttx::Type::Function;
};

}  // namespace Tetrodotoxin::Isa::Shader
