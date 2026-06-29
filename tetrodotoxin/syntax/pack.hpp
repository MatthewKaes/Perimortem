// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/syntax/expression/expression.hpp"

namespace Tetrodotoxin::Syntax {

using ExpressionNode = Tetrodotoxin::Syntax::Expression::Expression;

class Pack : public ExpressionNode {
 public:
  class Field {
   public:
    Lexical::Class klass = Lexical::Class::Type::EndOfStream;
    Perimortem::Core::View::Bytes name;
    Perimortem::Core::View::Bytes index;
    ExpressionNode* value = nullptr;
  };

  Pack(Context& ctx, Perimortem::Core::View::Vector<Field> fields);

  static auto parse(Context& ctx) -> Pack*;
  static auto parse_fields(Context& ctx)
      -> Perimortem::Core::View::Vector<Field>;
  static auto has_named_field(Perimortem::Core::View::Vector<Field> fields)
      -> Bool;
  static auto has_indexed_field(Perimortem::Core::View::Vector<Field> fields)
      -> Bool;

  auto get_kind() const -> Tetrodotoxin::Syntax::Expression::Kind override {
    return Tetrodotoxin::Syntax::Expression::Kind::Pack;
  }
  auto get_args() const
      -> Perimortem::Core::View::Vector<ExpressionNode*> override {
    return values;
  }
  auto get_fields() const -> Perimortem::Core::View::Vector<Field> {
    return fields;
  }

 private:
  Perimortem::Core::View::Vector<Field> fields;
  Perimortem::Core::View::Vector<ExpressionNode*> values;
};

}  // namespace Tetrodotoxin::Syntax
