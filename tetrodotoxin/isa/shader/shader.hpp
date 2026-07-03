// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/isa/context.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa {

// Shader is the installed body ISA for shader source files.
//
// Shader-specific validation and lowering will grow here. For now it makes
// `dialect : Shader;` an explicit toolchain capability so imports can be
// checked against the active registry.
class Shader {
 public:
  static auto evaluate(Context& context, Ttx::Lexical::Cursor& cursor) -> Bool;

  static constexpr auto get_name() -> Perimortem::Core::View::Bytes {
    return "Shader"_view;
  }
};

}  // namespace Tetrodotoxin::Isa
