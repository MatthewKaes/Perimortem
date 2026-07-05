// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa {

class Expression {
 public:
  static constexpr auto is_index_start(Ttx::Lexical::Class::Type type)
      -> Bool {
    switch (type) {
    case Ttx::Lexical::Class::Type::IndexStart:
    case Ttx::Lexical::Class::Type::SliceOp:
    case Ttx::Lexical::Class::Type::SwizzleOp:
      return True;
    default:
      return False;
    }
  }

  static constexpr auto is_index_start(Ttx::Lexical::Class type) -> Bool {
    return is_index_start(type.get_type());
  }

  static auto consume(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Core::View::Bytes error_message) -> Bool;
  static auto consume_initializer(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Core::View::Bytes error_message) -> Bool;
};

}  // namespace Tetrodotoxin::Isa
