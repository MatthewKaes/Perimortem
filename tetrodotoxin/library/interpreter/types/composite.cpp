// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/types/composite.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/interpreter/member.hpp"

using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library;

auto Interpreter::Types::Composite::parse_body(
    Cursor& cursor,
    Language::Types::Structure& structure,
    Tetrodotoxin::Language::Definition& definition,
    Token kind_token) -> Bool {
  BAIL_IF(!cursor.require(
      Code::Type::ScopeStart,
      "Library Composite bodies require an opening `{`."_view));

  // The real Type hosts each member as soon as its qualifier selects a
  // category. Later declarations can therefore observe stable names even when
  // one earlier edge remains unresolved.
  while (!cursor.matches(Code::Type::ScopeEnd)) {
    if (cursor.matches(Code::Type::Terminal)) {
      cursor.create_token_error(
          "Library Composite body reached the end of source before `}`."_view);
      return False;
    }

    const Ttx::Concept::Documentation& documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);
    auto nested = Tetrodotoxin::Language::Definition::parse(
        cursor, documentation, structure);
    BAIL_IF(!nested);
    auto member = Interpreter::Member::parse(cursor, *nested);
    BAIL_IF(!member);
    BAIL_IF(!structure.retain_authored_definition(
        member->get_semantic(), *nested, member->get_category(), cursor));
  }

  Token closing = cursor.consume();
  BAIL_IF(!definition.complete(kind_token, closing));
  structure.complete_authored_body();
  return True;
}
