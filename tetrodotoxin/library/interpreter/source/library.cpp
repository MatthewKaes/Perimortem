// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/source/library.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/interpreter/member.hpp"
#include "tetrodotoxin/library/interpreter/source/foreign.hpp"
#include "tetrodotoxin/library/interpreter/source/import.hpp"

using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library;

auto Interpreter::Source::Library::parse(
    Language::Types::Source& source,
    Cursor& cursor) -> Bool {
  Bool valid = True;
  while (!cursor.matches(Code::Type::Terminal)) {
    const Documentation& documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);

    if (cursor.matches(Code::Type::Using)) {
      auto import = Import::parse(cursor, documentation);
      if (!import || !source.retain_authored_import(*import)) {
        valid = False;
      }
      continue;
    }

    if (Foreign::is_next(cursor)) {
      valid &= Foreign::parse(source.get_foreign(), cursor, documentation);
      continue;
    }

    auto definition = Tetrodotoxin::Language::Definition::parse(
        cursor, documentation, source);
    if (!definition) {
      valid = False;
      cursor.recover_to_statement();
      continue;
    }

    auto member = Interpreter::Member::parse(cursor, *definition);
    if (!member) {
      valid = False;
      cursor.recover_to_statement();
      continue;
    }
    if (!source.retain_authored_definition(
            member->get_semantic(), *definition, member->get_category(),
            cursor)) {
      valid = False;
    }
  }
  return valid;
}
