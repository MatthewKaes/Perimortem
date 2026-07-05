// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/isa/context.hpp"
#include "ttx/documentation.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Scene {

// Lifecycle owns Scene's reserved root addressables.
class Lifecycle {
 public:
  static auto is_root(Perimortem::Core::View::Bytes name) -> Bool;
  static auto evaluate(
      Tetrodotoxin::Isa::Context& context,
      Ttx::Lexical::Cursor& cursor,
      Ttx::Documentation documentation) -> Ttx::Type::Function;
};

}  // namespace Tetrodotoxin::Isa::Scene
