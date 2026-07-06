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
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Isa::Context& context) -> const Ttx::Type*;
  static auto validate_stage(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Type& contract,
      const Ttx::Type::Function& function) -> Bool;

 private:
  static constexpr auto is_read_root(Perimortem::Core::View::Bytes name)
      -> Bool {
    return name == "constant"_view || name == "push"_view ||
           name == "resource"_view;
  }
  static auto validate_reads(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Type& contract,
      const Ttx::Type::Function& function) -> Bool;
};

}  // namespace Tetrodotoxin::Isa::Shader
