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
  switch (cursor.current().get_code().get_type()) {
  case Code::Type::State: {
    const Token start = cursor.current();
    cursor.consume();
    const Token* name = cursor.require(
        Code::Type::Addressable, "Expected shader state name."_view);
    if (name == nullptr) {
      return Statement();
    }

    Bool has_definition = cursor.require(
        Code::Type::Define, "Expected `:` after shader state name."_view);
    if (!has_definition) {
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

    Bool has_assignment = cursor.require(
        Code::Type::Assign,
        "Expected `=` before shader state initializer."_view);
    if (!has_assignment) {
      return Statement();
    }

    Base::Expression::Value initializer =
        Base::Expression::Value::evaluate(cursor, context);
    if (initializer.is_empty()) {
      return Statement();
    }

    const Token* end = cursor.require(
        Code::Type::EndStatement,
        "Expected `;` after shader state declaration."_view);
    if (end == nullptr) {
      return Statement();
    }

    return state_statement(start, *end, name->get_text(), *type, initializer);
  }

  case Code::Type::Return: {
    const Token start = cursor.current();
    cursor.consume();
    if (cursor.matches(Code::Type::EndStatement)) {
      const Token* end = cursor.require(
          Code::Type::EndStatement, "Expected `;` after shader return."_view);
      if (end == nullptr) {
        return Statement();
      }

      return return_statement(start, *end);
    }

    const Base::Expression::Pack* pack =
        Base::Expression::Pack::evaluate(cursor, context);
    if (pack == nullptr) {
      return Statement();
    }

    const Token* end = cursor.require(
        Code::Type::EndStatement, "Expected `;` after shader return."_view);
    if (end == nullptr) {
      return Statement();
    }

    return return_statement(start, *end, *pack);
  }

  default:
    cursor.token_error("Expected shader function statement."_view);
    return Statement();
  }
}
