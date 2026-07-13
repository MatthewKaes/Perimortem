// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/shader/statement.hpp"

#include "tetrodotoxin/isa/base/expression/type.hpp"
#include "ttx/lexical/cursor.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Shader::Statement::evaluate(Cursor& cursor, Base::Context& context)
    -> Statement {
  switch (cursor.current().get_class().get_type()) {
  case Class::Type::State: {
    const Token start = cursor.current();
    cursor.consume();
    const Token* name = cursor.require(
        Class::Type::Addressable, "Expected shader state name."_view);
    if (name == nullptr) {
      return Statement();
    }

    if (!cursor.require(
            Class::Type::Define,
            "Expected `:` after shader state name."_view)) {
      return Statement();
    }

    const Count error_count = cursor.get_errors().get_size();
    const Ttx::Type* type = Base::Expression::Type::evaluate(cursor, context);
    if (cursor.get_errors().get_size() != error_count) {
      return Statement();
    }

    if (type == nullptr) {
      cursor.token_error("Shader state type could not be resolved."_view);
      return Statement();
    }

    if (!cursor.require(
            Class::Type::Assign,
            "Expected `=` before shader state initializer."_view)) {
      return Statement();
    }

    Base::Expression::Value initializer =
        Base::Expression::Value::evaluate(cursor, context);
    if (initializer.is_empty()) {
      return Statement();
    }

    const Token* end = cursor.require(
        Class::Type::EndStatement,
        "Expected `;` after shader state declaration."_view);
    if (end == nullptr) {
      return Statement();
    }

    return state_statement(start, *end, name->get_text(), *type, initializer);
  }

  case Class::Type::Return: {
    const Token start = cursor.current();
    cursor.consume();
    const Base::Expression::Pack* pack = nullptr;
    if (!cursor.matches(Class::Type::EndStatement)) {
      pack = Base::Expression::Pack::evaluate(cursor, context);
      if (pack == nullptr) {
        return Statement();
      }
    }

    const Token* end = cursor.require(
        Class::Type::EndStatement, "Expected `;` after shader return."_view);
    if (end == nullptr) {
      return Statement();
    }

    return return_statement(start, *end, pack);
  }

  default:
    cursor.token_error("Expected shader function statement."_view);
    return Statement();
  }
}
