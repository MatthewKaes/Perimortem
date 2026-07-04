// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/isa/context.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa::Render {

// Render is the installed body ISA for render pipeline source files.
//
// Render-specific validation will eventually live here. For now the evaluator
// publishes the first authored Type as a shallow source export so package and
// Library code can resolve `ImportName::RenderType` without a side IR.
class VirtualMachine {
 public:
  static auto evaluate(
      Tetrodotoxin::Isa::Context& context,
      Ttx::Lexical::Cursor& cursor) -> Ttx::Type*;

  static constexpr auto get_name() -> Perimortem::Core::View::Bytes {
    return "Render"_view;
  }
};

}  // namespace Tetrodotoxin::Isa::Render
