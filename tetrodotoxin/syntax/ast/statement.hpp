// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/lexical/class.hpp"
#include "tetrodotoxin/syntax/ast/attribute.hpp"
#include "tetrodotoxin/syntax/ast/layout.hpp"
#include "tetrodotoxin/syntax/context.hpp"
#include "tetrodotoxin/syntax/expression/expression.hpp"
#include "tetrodotoxin/syntax/type/ref.hpp"

namespace Tetrodotoxin::Syntax::Ast {

// Arena-allocated statement node covering all statement forms in a function
// body. Sub-statements and expressions are arena-allocated in the same arena.
//
class Statement {
 public:
  // A single arm of a match statement: `case pattern : { body }`
  class MatchCase {
   public:
    Expression::Expression* pattern;  // nullptr means wildcard `_`
    Perimortem::Core::View::Vector<Statement*> body;
  };

  static auto parse(Context& ctx) -> Statement*;

  auto get_class() const -> Lexical::Class { return klass; }
  auto get_name() const -> Perimortem::Core::View::Bytes { return name; }
  auto get_sigil() const -> Lexical::Class { return sigil; }
  auto get_type_ref() const -> const Type::Ref& { return type_ref; }
  auto get_layout() const -> const Layout& { return layout; }
  auto get_operator() const -> Lexical::Class { return operator_class; }
  auto get_cond() const -> const Expression::Expression* { return cond; }
  auto get_value() const -> const Expression::Expression* { return value; }

  auto get_body() const -> Perimortem::Core::View::Vector<Statement*> {
    return body;
  }

  auto get_else_body() const -> Perimortem::Core::View::Vector<Statement*> {
    return else_body;
  }

  auto get_cases() const -> Perimortem::Core::View::Vector<MatchCase> {
    return cases;
  }

  auto get_attributes() const -> Perimortem::Core::View::Vector<Attribute> {
    return attributes;
  }

  auto is_declaration() const -> Bool { return klass.is_sigil(); }

  auto is_aligned_declaration() const -> Bool {
    return is_declaration() && operator_class == Lexical::Class::Type::Define;
  }

  auto is_block() const -> Bool {
    return klass == Lexical::Class::Type::If ||
           klass == Lexical::Class::Type::CompileIf ||
           klass == Lexical::Class::Type::For ||
           klass == Lexical::Class::Type::While ||
           klass == Lexical::Class::Type::Match;
  }

 private:
  class Parser;
  friend class Parser;

  explicit Statement(Lexical::Class klass);

  Lexical::Class klass = Lexical::Class::Type::EndOfStream;
  Lexical::Class sigil = Lexical::Class::Type::EndOfStream;
  Lexical::Class operator_class = Lexical::Class::Type::EndOfStream;
  Perimortem::Core::View::Bytes name;
  Type::Ref type_ref;
  Layout layout;
  Expression::Expression* cond = nullptr;
  Expression::Expression* value = nullptr;
  Perimortem::Core::View::Vector<Statement*> body;
  Perimortem::Core::View::Vector<Statement*> else_body;
  Perimortem::Core::View::Vector<MatchCase> cases;
  Perimortem::Core::View::Vector<Attribute> attributes;
};

using MatchCase = Statement::MatchCase;

}  // namespace Tetrodotoxin::Syntax::Ast
