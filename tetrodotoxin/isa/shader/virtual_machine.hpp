// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/isa/base/context.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa::Lowering {

class Context;
struct Input;

}  // namespace Tetrodotoxin::Isa::Lowering

namespace Tetrodotoxin::Isa::Shader {

// Shader is the installed body ISA for shader source files.
//
// Shader-specific validation and lowering will grow here. For now the
// evaluator publishes the first authored Type as a shallow source export so the
// rest of the VM cluster can still use normal Ttx::Type queries.
class VirtualMachine {
 public:
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Isa::Base::Context& context) -> Ttx::Type*;
  static auto lower(
      Tetrodotoxin::Isa::Lowering::Context& context,
      const Tetrodotoxin::Isa::Lowering::Input& input) -> Bool;

  static constexpr auto get_name() -> Perimortem::Core::View::Bytes {
    return "Shader"_view;
  }
};

}  // namespace Tetrodotoxin::Isa::Shader
