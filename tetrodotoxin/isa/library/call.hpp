// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/isa/expression/pack.hpp"
#include "ttx/type.hpp"

namespace Ttx::Lexical {
class Cursor;
}

namespace Tetrodotoxin::Isa::Library {

class Scope;

class Call {
 public:
  constexpr Call() = default;
  constexpr Call(
      const Ttx::Type& owner,
      const Ttx::Type::Function& function,
      Perimortem::Core::View::Bytes name,
      Expression::Pack pack)
      : owner(&owner), function(&function), name(name), pack(pack) {}

  static auto evaluate(Ttx::Lexical::Cursor& cursor, Scope& scope)
      -> const Call*;

  constexpr auto get_owner() const -> const Ttx::Type* { return owner; }
  constexpr auto get_function() const -> const Ttx::Type::Function* {
    return function;
  }
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }
  constexpr auto get_pack() const -> Expression::Pack { return pack; }

 private:
  const Ttx::Type* owner = nullptr;
  const Ttx::Type::Function* function = nullptr;
  Perimortem::Core::View::Bytes name;
  Expression::Pack pack;
};

}  // namespace Tetrodotoxin::Isa::Library
