// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/library/function.hpp"

#include "tetrodotoxin/isa/layout/evaluator.hpp"
#include "tetrodotoxin/isa/library/block.hpp"
#include "tetrodotoxin/isa/library/statement.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Library::Function::evaluate(
    Cursor& cursor,
    Scope& scope,
    Ttx::Documentation documentation) -> Ttx::Type::Function {
  return evaluate(cursor, scope, documentation, BodyMode::Definition);
}

auto Library::Function::evaluate_declaration(
    Cursor& cursor,
    Scope& scope,
    Ttx::Documentation documentation) -> Ttx::Type::Function {
  return evaluate(cursor, scope, documentation, BodyMode::Declaration);
}

auto Library::Function::evaluate(
    Cursor& cursor,
    Scope& scope,
    Ttx::Documentation documentation,
    BodyMode body_mode) -> Ttx::Type::Function {
  if (!cursor.require(
          Class::Type::Func, "Expected `func` in library function."_view)) {
    return Ttx::Type::Function();
  }

  const Token* name = cursor.require(
      Class::Type::Addressable, "Expected library function name."_view);
  if (name == nullptr) {
    return Ttx::Type::Function();
  }

  Managed::Vector<Ttx::Type::Member> parameters(
      scope.get_context().get_arena());
  if (!Layout::Evaluator::evaluate_bracketed(cursor, scope, parameters)) {
    return Ttx::Type::Function();
  }

  if (!cursor.require(
          Class::Type::CallOp,
          "Expected `->` before library function result."_view)) {
    return Ttx::Type::Function();
  }

  Managed::Vector<Ttx::Type::Member> result(scope.get_context().get_arena());
  if (!Layout::Evaluator::evaluate(cursor, scope, result)) {
    return Ttx::Type::Function();
  }

  Managed::Vector<Ttx::Type::Function::Block> blocks(
      scope.get_context().get_arena());
  if (body_mode == BodyMode::Declaration) {
    if (!cursor.require(
            Class::Type::EndStatement,
            "Expected `;` after library function declaration."_view)) {
      return Ttx::Type::Function();
    }
  } else if (!evaluate_body(cursor, scope, blocks)) {
    return Ttx::Type::Function();
  }

  return Ttx::Type::Function(
      name->get_text(), parameters.get_view(), result.get_view(),
      blocks.get_view(), documentation);
}

auto Library::Function::evaluate_body(
    Cursor& cursor,
    Scope& scope,
    Managed::Vector<Ttx::Type::Function::Block>& blocks) -> Bool {
  if (!cursor.require(
          Class::Type::ScopeStart,
          "Expected `{` after library function signature."_view)) {
    return False;
  }

  Managed::Vector<Library::Statement> statements(
      scope.get_context().get_arena());
  while (!cursor.matches(Class::Type::EndOfStream)) {
    if (cursor.matches(Class::Type::ScopeEnd)) {
      cursor.consume();
      auto& block = scope.get_context().get_arena()
                        .construct<Library::Block>(statements.get_view());
      blocks.insert(Ttx::Type::Function::Block(block));
      return True;
    }

    Library::Statement::Result result =
        Library::Statement::evaluate(cursor, scope);
    switch (result.get_kind()) {
    case Library::Statement::Result::Kind::Ignored:
      break;
    case Library::Statement::Result::Kind::Ready:
      statements.insert(result.get_statement());
      break;
    default:
      return False;
    }
  }

  cursor.token_error("Expected `}` after library function body."_view);
  return False;
}
