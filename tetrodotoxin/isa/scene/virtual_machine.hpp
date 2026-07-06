// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/isa/context.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa::Scene {

// Scene is the body ISA for app scene sources.
//
// It records state, constants, helper functions, and the reserved lifecycle
// roots `on_start`, `on_update`, and `on_exit`.
class VirtualMachine {
 public:
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Isa::Context& context) -> Ttx::Type*;

  static constexpr auto get_name() -> Perimortem::Core::View::Bytes {
    return "Scene"_view;
  }
};

}  // namespace Tetrodotoxin::Isa::Scene
