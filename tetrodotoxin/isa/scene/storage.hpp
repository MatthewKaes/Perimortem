// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/base/context.hpp"
#include "tetrodotoxin/isa/base/definition.hpp"
#include "ttx/documentation.hpp"
#include "ttx/lexical/code.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Scene {

// Storage owns Scene state and constant declarations.
class Storage {
 public:
  static constexpr auto is_modifier(Ttx::Lexical::Code::Type type) -> Bool {
    switch (type) {
    case Ttx::Lexical::Code::Type::State:
    case Ttx::Lexical::Code::Type::Const:
      return True;
    default:
      return False;
    }
  }

  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Isa::Base::Context& context,
      Ttx::Documentation documentation,
      Ttx::Lexical::Code::Type storage,
      Tetrodotoxin::Isa::Base::Definition& implementation)
      -> const Ttx::Member*;
  static auto insert(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Memory::Managed::Vector<Ttx::Member>& members,
      Ttx::Member member) -> Bool;
};

}  // namespace Tetrodotoxin::Isa::Scene
