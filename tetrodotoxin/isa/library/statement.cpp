// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/library/statement.hpp"

#include "tetrodotoxin/isa/expression.hpp"
#include "tetrodotoxin/isa/library/scope.hpp"
#include "ttx/lexical/cursor.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Library::Statement::evaluate(Cursor& cursor, Scope& scope) -> Result {
  switch (cursor.current().get_class().get_type()) {
  case Class::Type::Comment:
  case Class::Type::Disabled:
    cursor.consume();
    return Result::ignored();
  case Class::Type::Return:
    cursor.consume();
    if (!cursor.matches(Class::Type::EndStatement) &&
        !Expression::consume(cursor, "Expected `;` after return value."_view)) {
      return Result::failed();
    }

    if (!cursor.require(
            Class::Type::EndStatement, "Expected `;` after return."_view)) {
      return Result::failed();
    }

    return Result::ready(return_statement());
  case Class::Type::Type: {
    const Call* call = Call::evaluate(cursor, scope);
    if (call == nullptr) {
      return Result::failed();
    }

    if (!cursor.require(
            Class::Type::EndStatement,
            "Expected `;` after library call."_view)) {
      return Result::failed();
    }

    return Result::ready(call_statement(*call));
  }
  default:
    cursor.token_error("Expected library function statement."_view);
    return Result::failed();
  }
}
