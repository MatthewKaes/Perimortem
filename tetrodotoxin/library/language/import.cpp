// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/import.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Utility;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

auto Library::Language::Import::parse(Cursor& cursor) -> Option<Import> {
  Token opening = cursor.require(
      Code::Type::Using, "Library Imports require `using`."_view);
  if (!opening) {
    cursor.recover_to_statement();
    return {};
  }

  auto type_reference = TypeReference::parse(cursor);
  if (!type_reference) {
    cursor.recover_to_statement();
    return {};
  }

  // The exact Tokens and stable segment spellings survive the parser. Waiting
  // for the terminator keeps malformed trailing syntax out of durable state.
  Token terminator = cursor.require(
      Code::Type::EndStatement,
      "Library Imports require one terminating `;`."_view);
  if (!terminator) {
    cursor.recover_to_statement();
    return {};
  }

  return Import(*type_reference, Span(opening, terminator));
}
