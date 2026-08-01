// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/import.hpp"

#include "tetrodotoxin/package/language/parser/name.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Utility;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

auto Library::Language::Import::parse(Cursor& cursor) -> Option<Import> {
  Token opening = cursor.require(
      Code::Type::Addressable,
      "Library Imports require exact `using` spelling."_view);
  if (!opening) {
    cursor.recover_to_statement();
    return {};
  }

  if (opening.caculate_text(cursor.get_source_text()) != "using"_view) {
    cursor.create_token_error(
        opening, "Library Imports require exact `using` spelling."_view);
    cursor.recover_to_statement();
    return {};
  }

  View::Bytes route = Package::Language::Parser::Name::parse_semantic(cursor);
  if (route.is_empty()) {
    cursor.recover_to_statement();
    return {};
  }

  // The route stays borrowed from the retained source. Waiting for the
  // terminator keeps malformed trailing syntax out of durable Import state.
  Token terminator = cursor.require(
      Code::Type::EndStatement,
      "Library Imports require one terminating `;`."_view);
  if (!terminator) {
    cursor.recover_to_statement();
    return {};
  }

  return Import(route);
}
