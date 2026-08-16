// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/model/parser/layout.hpp"

using namespace Perimortem;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library;

auto Language::Model::Parser::Layout::require_shape(
    Cursor& cursor,
    Core::Option<Bool>& selected,
    Bool named) -> Bool {
  if (!selected) {
    selected = named;
    return True;
  }

  if (*selected == named) {
    return True;
  }

  cursor.create_token_error(
      "Positional and named entries cannot share one Library Layout."_view);
  return False;
}

auto Language::Model::Parser::Layout::retain_name(
    Cursor& cursor,
    Token token,
    Memory::Managed::Vector<Core::View::Bytes>& names) -> Bool {
  Core::View::Bytes name = token.caculate_text(cursor.get_source_text());
  if (names.get_view().contains(
          [&](Core::View::Bytes retained) { return retained == name; })) {
    cursor.create_token_error(
        token, "Duplicate name in one Library Layout."_view);
    return False;
  }

  // The parsed model retains this same view into the source for delayed linking
  // and reflection because its transaction Arena keeps the source alive.
  names.insert(name);
  return True;
}
