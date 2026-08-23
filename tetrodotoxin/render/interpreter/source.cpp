// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/render/interpreter/source.hpp"

using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Render;

auto Interpreter::Source::parse(Cursor& cursor) -> void {
  while (!cursor.matches(Code::Type::Terminal)) {
    cursor.create_token_error(
        "Render does not recognize this declaration yet."_view,
        "Render contracts will add their declaration owners to this source "
        "boundary."_view);
    cursor.recover_to_statement();
  }
}
