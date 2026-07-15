// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/base/context.hpp"
#include "tetrodotoxin/isa/shader/block.hpp"
#include "ttx/documentation.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Shader {

// Function owns shader stage syntax.
class Function {
 public:
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Isa::Base::Context& context,
      Ttx::Documentation documentation,
      const Block*& block) -> Ttx::Function;
};

}  // namespace Tetrodotoxin::Isa::Shader
