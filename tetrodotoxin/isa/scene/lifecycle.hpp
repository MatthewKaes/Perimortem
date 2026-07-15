// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/base/context.hpp"
#include "ttx/documentation.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Scene {

// Lifecycle owns Scene's reserved root addressables.
class Lifecycle {
 public:
  static auto is_root(Perimortem::Core::View::Bytes name) -> Bool;
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Isa::Base::Context& context,
      Ttx::Documentation documentation,
      const Ttx::Type* owner) -> Ttx::Function;
};

}  // namespace Tetrodotoxin::Isa::Scene
