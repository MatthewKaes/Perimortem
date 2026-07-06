// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/isa/definition.hpp"
#include "tetrodotoxin/isa/library/scope.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Library {

// Addressable owns Library definitions whose authored name is an addressable
// value. In the Library ISA that means a member entry on the surrounding type.
class Addressable {
 public:
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Scope& scope,
      const Tetrodotoxin::Isa::Definition& definition) -> Ttx::Type::Member;
};

}  // namespace Tetrodotoxin::Isa::Library
