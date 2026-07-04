// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "ttx/lexical/class.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa {

// Modifier is a shared ISA syntax helper for the common access and storage
// words. It intentionally stores the token class instead of inventing a second
// enum so the real semantics can later move onto the TTX data model.
class Modifier {
 public:
  Modifier() = default;
  explicit Modifier(Ttx::Lexical::Class::Type type)
      : type(type), valid(True) {}

  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Core::View::Vector<Ttx::Lexical::Class::Type> allowed,
      Perimortem::Core::View::Bytes error_message) -> Modifier;

  constexpr auto get_type() const -> Ttx::Lexical::Class::Type {
    return type;
  }
  constexpr auto is_valid() const -> Bool { return valid; }

 private:
  Ttx::Lexical::Class::Type type = Ttx::Lexical::Class::Type::Unknown;
  Bool valid = False;
};

}  // namespace Tetrodotoxin::Isa
