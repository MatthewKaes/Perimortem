// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/isa/base/context.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Shader {

class Block;

// Contract owns the Shader-to-Render handshake.
class Contract {
 public:
  static auto resolve(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Isa::Base::Context& context) -> const Ttx::Type*;
  static auto validate_stage(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Type& contract,
      const Ttx::Function& function,
      const Block& block) -> Bool;

 private:
  static auto validate_reads(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Type& contract,
      const Ttx::Function& function,
      const Block& block) -> Bool;
};

}  // namespace Tetrodotoxin::Isa::Shader
