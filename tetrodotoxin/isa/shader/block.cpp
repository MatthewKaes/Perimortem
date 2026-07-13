// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/shader/block.hpp"

#include "ttx/lexical/cursor.hpp"

using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Shader::Block::evaluate(Cursor& cursor, Base::Context& context)
    -> const Block* {
  if (!cursor.require(
          Class::Type::ScopeStart,
          "Expected `{` after shader function signature."_view)) {
    return nullptr;
  }

  Managed::Vector<Shader::Statement> statements(context.get_arena());
  while (!cursor.matches(Class::Type::EndOfStream)) {
    if (cursor.matches(Class::Type::ScopeEnd)) {
      cursor.consume();
      return &context.get_arena().construct<Shader::Block>(
          statements.get_view());
    }

    if (cursor.is_one_of({{Class::Type::Comment, Class::Type::Disabled}})) {
      cursor.consume();
      continue;
    }

    Shader::Statement statement = Shader::Statement::evaluate(cursor, context);
    if (statement.is_empty()) {
      return nullptr;
    }

    statements.insert(statement);
  }

  cursor.token_error("Expected `}` after shader function body."_view);
  return nullptr;
}
