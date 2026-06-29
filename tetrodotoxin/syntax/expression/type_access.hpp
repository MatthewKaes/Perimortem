// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/syntax/expression/expression.hpp"

namespace Tetrodotoxin::Syntax::Expression {

class TypeAccess : public Expression {
 public:
  explicit TypeAccess(Ref type_ref)
      : Expression(Lexical::Class::Type::Type, type_ref.get_name()),
        type_ref(type_ref) {}

  auto get_kind() const -> Kind override { return Kind::TypeAccess; }
  auto get_type_ref() const -> const Ref& override { return type_ref; }

 private:
  Ref type_ref;
};

}  // namespace Tetrodotoxin::Syntax::Expression
