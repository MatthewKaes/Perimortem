// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/shader/statement.hpp"

#include "tetrodotoxin/isa/expression/type.hpp"
#include "ttx/lexical/cursor.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Shader::Statement::evaluate(Cursor& cursor, Context& context) -> Result {
  switch (cursor.current().get_class().get_type()) {
  case Class::Type::Comment:
  case Class::Type::Disabled:
    cursor.consume();
    return Result::ignored();

  case Class::Type::State: {
    const Token start = cursor.current();
    cursor.consume();
    const Token* name = cursor.require(
        Class::Type::Addressable, "Expected shader state name."_view);
    if (name == nullptr) {
      return Result::failed();
    }

    if (!cursor.require(
            Class::Type::Define, "Expected `:` after shader state name."_view)) {
      return Result::failed();
    }

    const Count error_count = cursor.get_errors().get_size();
    const Ttx::Type* type = Expression::Type::evaluate(cursor, context);
    if (cursor.get_errors().get_size() != error_count) {
      return Result::failed();
    }

    if (type == nullptr) {
      cursor.token_error("Shader state type could not be resolved."_view);
      return Result::failed();
    }

    if (!cursor.require(
            Class::Type::Assign,
            "Expected `=` before shader state initializer."_view)) {
      return Result::failed();
    }

    Expression::Value initializer =
        Expression::Value::evaluate(cursor, context);
    if (initializer.is_empty()) {
      return Result::failed();
    }

    const Token* end = cursor.require(
        Class::Type::EndStatement,
        "Expected `;` after shader state declaration."_view);
    if (end == nullptr) {
      return Result::failed();
    }

    return Result::ready(state_statement(
        start, *end, name->get_text(), *type, initializer));
  }

  case Class::Type::Return: {
    const Token start = cursor.current();
    cursor.consume();
    const Expression::Pack* pack = nullptr;
    if (!cursor.matches(Class::Type::EndStatement)) {
      pack = Expression::Pack::evaluate(cursor, context);
      if (pack == nullptr) {
        return Result::failed();
      }
    }

    const Token* end = cursor.require(
        Class::Type::EndStatement, "Expected `;` after shader return."_view);
    if (end == nullptr) {
      return Result::failed();
    }

    return Result::ready(return_statement(start, *end, pack));
  }

  default:
    cursor.token_error("Expected shader function statement."_view);
    return Result::failed();
  }
}
