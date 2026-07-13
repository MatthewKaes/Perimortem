// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/base/context.hpp"
#include "tetrodotoxin/isa/base/declaration.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Render {

class StageResult;

// Stage owns render stage declarations and the shader-visible reads each stage
// exposes from Render fact blocks.
class Stage {
 public:
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Isa::Base::Context& context,
      const Tetrodotoxin::Isa::Base::Declaration& definition,
      Perimortem::Core::View::Vector<const Ttx::Type*> facts) -> StageResult;
  static auto insert(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Memory::Managed::Vector<Ttx::Function>& functions,
      Ttx::Function stage) -> Bool;

 private:
  static auto consume_reads(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Isa::Base::Context& context,
      Perimortem::Core::View::Vector<const Ttx::Type*> facts,
      Perimortem::Memory::Managed::Vector<Ttx::Member>& constants,
      Perimortem::Memory::Managed::Vector<Ttx::Member>& pushes,
      Perimortem::Memory::Managed::Vector<Ttx::Member>& resources) -> Bool;
  static auto build_facts(
      Tetrodotoxin::Isa::Base::Context& context,
      Perimortem::Core::View::Bytes stage_name,
      Perimortem::Memory::Managed::Vector<Ttx::Member>& constants,
      Perimortem::Memory::Managed::Vector<Ttx::Member>& pushes,
      Perimortem::Memory::Managed::Vector<Ttx::Member>& resources)
      -> const Ttx::Type*;
};

}  // namespace Tetrodotoxin::Isa::Render
