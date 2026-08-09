// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/language/parser/dialect.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "ttx/lexical/lexicon.hpp"
#include "ttx/lexical/token.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Language;

auto Parser::Dialect::parse(Cursor& cursor) -> View::Bytes {
  if (!cursor.require(
          Code::Type::Dialect,
          "Source files are required to select a dialect using `dialect : Type;`."_view)) {
    return View::Bytes();
  }

  if (!cursor.require(
          Code::Type::Define,
          "Expected `:` after the source Dialect declaration."_view)) {
    return View::Bytes();
  }

  Token dialect_token = cursor.require(
      Code::Type::Type,
      "Expected a concrete source Dialect name such as `Package` or `Library`."_view);
  if (!dialect_token) {
    return View::Bytes();
  }

  View::Bytes dialect_name =
      dialect_token.caculate_text(cursor.get_source_text());
  if (!cursor.require(
          Code::Type::EndStatement,
          "Expected `;` after the source Dialect declaration."_view)) {
    return View::Bytes();
  }

  return dialect_name;
}
