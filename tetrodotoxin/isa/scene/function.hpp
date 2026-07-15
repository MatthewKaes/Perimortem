// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/base/context.hpp"
#include "ttx/documentation.hpp"
#include "ttx/lexical/class.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Scene {

// Function owns normal Scene helper functions.
class Function {
 public:
  static constexpr auto is_modifier(Ttx::Lexical::Class::Type type) -> Bool {
    switch (type) {
    case Ttx::Lexical::Class::Type::Public:
    case Ttx::Lexical::Class::Type::Private:
      return True;
    default:
      return False;
    }
  }

  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Isa::Base::Context& context,
      Ttx::Documentation documentation,
      const Ttx::Type* owner,
      Bool& addressable) -> Ttx::Function;
  static auto insert(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Memory::Managed::Vector<Ttx::Function>& functions,
      Ttx::Function function) -> Bool;
};

}  // namespace Tetrodotoxin::Isa::Scene
