// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/isa/context.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa {

// Render is the installed body ISA for render pipeline source files.
//
// Render-specific validation will eventually live here. The current evaluator
// still makes `dialect : Render;` an explicit toolchain capability.
class Render {
 public:
  static auto evaluate(Context& context, Ttx::Lexical::Cursor& cursor) -> Bool;

  static constexpr auto get_name() -> Perimortem::Core::View::Bytes {
    return "Render"_view;
  }
};

}  // namespace Tetrodotoxin::Isa
