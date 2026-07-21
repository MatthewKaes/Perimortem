// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/library/function.hpp"

#include "tetrodotoxin/isa/base/layout/evaluator.hpp"
#include "tetrodotoxin/isa/library/syntax.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

enum class BodyMode : Unsigned_8 {
  Definition,
  Declaration,
};

static auto evaluate_function(
    Cursor& cursor,
    Library::Scope& scope,
    Ttx::Documentation documentation,
    BodyMode body_mode,
    Perimortem::Utility::Range& source,
    const Ttx::Type* owner,
    Bool* addressable) -> Ttx::Function {
  Bool has_function = cursor.require(
      Code::Type::Func, "Expected `func` in library function."_view);
  if (!has_function) {
    return Ttx::Function();
  }

  const Token* name = cursor.require(
      Code::Type::Addressable, "Expected library function name."_view);
  if (name == nullptr) {
    return Ttx::Function();
  }

  Managed::Vector<Ttx::Member> parameters(scope.get_context().get_arena());
  Bool parameters_evaluated = Base::Layout::Evaluator::evaluate_bracketed(
      cursor, scope, parameters, owner, addressable);
  if (!parameters_evaluated) {
    return Ttx::Function();
  }

  Bool has_call = cursor.require(
      Code::Type::CallOp, "Expected `->` before library function result."_view);
  if (!has_call) {
    return Ttx::Function();
  }

  Managed::Vector<Ttx::Member> result(scope.get_context().get_arena());
  Bool result_evaluated =
      Base::Layout::Evaluator::evaluate(cursor, scope, result);
  if (!result_evaluated) {
    return Ttx::Function();
  }

  Ttx::Function function(
      name->get_text(), Ttx::Layout(parameters.get_view()),
      Ttx::Layout(result.get_view()), documentation);
  if (body_mode == BodyMode::Declaration) {
    Bool has_statement_end = cursor.require(
        Code::Type::EndStatement,
        "Expected `;` after library function declaration."_view);
    if (!has_statement_end) {
      return Ttx::Function();
    }
  } else if (cursor.matches(Code::Type::EndStatement)) {
    cursor.consume();
  } else {
    Count start = cursor.get_token_index();
    if (!cursor.matches(Code::Type::ScopeStart)) {
      return Ttx::Function();
    }

    Bool consumed = Library::Syntax::consume_declaration_tail(cursor);
    if (!consumed) {
      return Ttx::Function();
    }

    Count end = cursor.get_token_index();
    View::Vector<Token> tail = cursor.get_token_span(end - 1, end);
    if (tail.is_empty() || tail[0].get_code() != Code::Type::ScopeEnd) {
      cursor.token_error("Expected `}` after library function body."_view);
      return Ttx::Function();
    }

    source = {start, end - start};
  }

  return function;
}

auto Library::Function::evaluate(
    Cursor& cursor,
    Scope& scope,
    Ttx::Documentation documentation,
    Perimortem::Utility::Range& source) -> Ttx::Function {
  return evaluate_function(
      cursor, scope, documentation, BodyMode::Definition, source, nullptr,
      nullptr);
}

auto Library::Function::evaluate(
    Cursor& cursor,
    Scope& scope,
    Ttx::Documentation documentation,
    Perimortem::Utility::Range& source,
    const Ttx::Type* owner,
    Bool& addressable) -> Ttx::Function {
  return evaluate_function(
      cursor, scope, documentation, BodyMode::Definition, source, owner,
      &addressable);
}

auto Library::Function::evaluate_declaration(
    Cursor& cursor,
    Scope& scope,
    Ttx::Documentation documentation,
    const Ttx::Type* owner) -> Ttx::Function {
  Perimortem::Utility::Range source;
  return evaluate_function(
      cursor, scope, documentation, BodyMode::Declaration, source, owner,
      nullptr);
}
