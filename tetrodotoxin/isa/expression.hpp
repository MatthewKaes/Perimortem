// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "ttx/type.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa {

// Expression owns the reusable expression micro-language shared by body ISAs.
//
// A pack is the parenthesized expression form that produces layout-shaped
// values for calls and future construction sites. ISAs such as Library and
// Shader decide where expressions are legal, then hand the expression span to
// this owner instead of inventing local value/pack models.
class Expression {
 public:
  class Pack;
  class Type;
  class Value;

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
  static auto consume_block(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Core::View::Bytes open_error,
      Perimortem::Core::View::Bytes close_error,
      Perimortem::Memory::Managed::Vector<Ttx::Type::Function::Block>& blocks)
      -> Bool;
};

}  // namespace Tetrodotoxin::Isa
