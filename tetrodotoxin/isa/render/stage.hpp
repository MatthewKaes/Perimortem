// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/context.hpp"
#include "tetrodotoxin/isa/definition.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Render {

// Stage owns render stage declarations and the shader-visible reads each stage
// exposes from Render fact blocks.
class Stage {
 public:
  class Result {
   public:
    constexpr Result() = default;
    constexpr Result(Ttx::Type::Function function, const Ttx::Type& facts)
        : function(function), facts(&facts) {}

    constexpr auto get_function() const -> Ttx::Type::Function {
      return function;
    }
    constexpr auto get_facts() const -> const Ttx::Type* { return facts; }
    constexpr auto is_empty() const -> Bool {
      return function.is_empty() || facts == nullptr;
    }

   private:
    Ttx::Type::Function function;
    const Ttx::Type* facts = nullptr;
  };

  static auto evaluate(
      Tetrodotoxin::Isa::Context& context,
      Ttx::Lexical::Cursor& cursor,
      const Tetrodotoxin::Isa::Definition& definition,
      Perimortem::Core::View::Vector<const Ttx::Type*> facts) -> Result;
  static auto insert(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Memory::Managed::Vector<Ttx::Type::Function>& functions,
      Ttx::Type::Function stage) -> Bool;

 private:
  static auto consume_reads(
      Tetrodotoxin::Isa::Context& context,
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Core::View::Vector<const Ttx::Type*> facts,
      Perimortem::Memory::Managed::Vector<Ttx::Type::Member>& constants,
      Perimortem::Memory::Managed::Vector<Ttx::Type::Member>& pushes,
      Perimortem::Memory::Managed::Vector<Ttx::Type::Member>& resources)
      -> Bool;
  static auto build_facts(
      Tetrodotoxin::Isa::Context& context,
      Perimortem::Core::View::Bytes stage_name,
      Perimortem::Memory::Managed::Vector<Ttx::Type::Member>& constants,
      Perimortem::Memory::Managed::Vector<Ttx::Type::Member>& pushes,
      Perimortem::Memory::Managed::Vector<Ttx::Type::Member>& resources)
      -> const Ttx::Type*;
};

}  // namespace Tetrodotoxin::Isa::Render
