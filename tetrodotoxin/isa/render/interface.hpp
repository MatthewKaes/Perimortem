// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/context.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Render {

// Interface owns Render's named data blocks: constants, push constants, and
// resources. They become nested TTX types so other ISAs can query them without a
// compiler-side mirror.
class Interface {
 public:
  static auto source_to_type_name(Perimortem::Core::View::Bytes block_name)
      -> Perimortem::Core::View::Bytes;
  static auto find(
      Perimortem::Core::View::Vector<const Ttx::Type*> types,
      Perimortem::Core::View::Bytes type_name) -> const Ttx::Type*;
  static auto evaluate(
      Tetrodotoxin::Isa::Context& context,
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Core::View::Bytes block_name) -> const Ttx::Type*;
  static auto insert(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Memory::Managed::Vector<Ttx::Type::Member>& members,
      Ttx::Type::Member member,
      Perimortem::Core::View::Bytes duplicate_error) -> Bool;
};

}  // namespace Tetrodotoxin::Isa::Render
