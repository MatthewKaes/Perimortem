// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/isa/context.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Shader {

// Contract owns the Shader-to-Render handshake.
class Contract {
 public:
  static auto resolve(
      Tetrodotoxin::Isa::Context& context,
      Ttx::Lexical::Cursor& cursor) -> const Ttx::Type*;
  static auto validate_stage(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Type& contract,
      const Ttx::Type::Function& function) -> Bool;
};

}  // namespace Tetrodotoxin::Isa::Shader
