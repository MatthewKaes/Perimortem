// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/context.hpp"
#include "ttx/documentation.hpp"
#include "ttx/lexical/class.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Scene {

// Storage owns Scene state and constant declarations.
class Storage {
 public:
  static constexpr auto is_modifier(Ttx::Lexical::Class::Type type) -> Bool {
    switch (type) {
    case Ttx::Lexical::Class::Type::State:
    case Ttx::Lexical::Class::Type::Const:
      return True;
    default:
      return False;
    }
  }

  static auto evaluate(
      Tetrodotoxin::Isa::Context& context,
      Ttx::Lexical::Cursor& cursor,
      Ttx::Documentation documentation,
      Ttx::Lexical::Class::Type storage) -> Ttx::Type::Member;
  static auto insert(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Memory::Managed::Vector<Ttx::Type::Member>& members,
      Ttx::Type::Member member) -> Bool;
  static auto build_fact_type(
      Tetrodotoxin::Isa::Context& context,
      Perimortem::Core::View::Bytes type_name,
      Perimortem::Core::View::Bytes block_name,
      Perimortem::Core::View::Vector<Ttx::Type::Member> members)
      -> const Ttx::Type*;
};

}  // namespace Tetrodotoxin::Isa::Scene
