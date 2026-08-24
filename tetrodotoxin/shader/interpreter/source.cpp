// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/shader/interpreter/source.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/shader/interpreter/program.hpp"

using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Shader;

auto Interpreter::Source::parse(
    Shader::Language::Monograph& monograph,
    Cursor& cursor) -> void {
  while (!cursor.matches(Code::Type::Terminal)) {
    const Documentation& documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);
    auto definition = Tetrodotoxin::Language::Definition::parse(
        cursor, documentation, monograph.edit_library().get_source());
    if (!definition ||
        !Interpreter::Program::parse(monograph, cursor, *definition)) {
      cursor.recover_to_statement();
    }
  }
}
