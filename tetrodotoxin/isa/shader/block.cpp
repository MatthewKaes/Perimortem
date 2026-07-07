// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/shader/block.hpp"

#include "ttx/lexical/cursor.hpp"

using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Shader::Block::evaluate(
    Cursor& cursor,
    Context& context,
    Managed::Vector<Ttx::Type::Function::Block>& blocks) -> Bool {
  if (!cursor.require(
          Class::Type::ScopeStart,
          "Expected `{` after shader function signature."_view)) {
    return False;
  }

  const Count token_start = cursor.get_token_index();
  Managed::Vector<Shader::Statement> statements(context.get_arena());
  while (!cursor.matches(Class::Type::EndOfStream)) {
    if (cursor.matches(Class::Type::ScopeEnd)) {
      const Count token_end = cursor.get_token_index();
      cursor.consume();
      auto& block =
          context.get_arena().construct<Shader::Block>(statements.get_view());
      blocks.insert(Ttx::Type::Function::Block(
          cursor.get_token_span(token_start, token_end), block));
      return True;
    }

    Shader::Statement::Result result =
        Shader::Statement::evaluate(cursor, context);
    switch (result.get_kind()) {
    case Shader::Statement::Result::Kind::Ignored:
      break;
    case Shader::Statement::Result::Kind::Ready:
      statements.insert(result.get_statement());
      break;
    default:
      return False;
    }
  }

  cursor.token_error("Expected `}` after shader function body."_view);
  return False;
}
