// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/build/dialect.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

auto Build::Dialect::interpret(
    Cursor& cursor,
    const Documentation&,
    const Anchor&,
    Abstract&) -> Option<Language::Monograph&> {
  // The host can install Build before its command language is available. An
  // explicit failure keeps a bootstrap invocation from claiming it built
  // output.
  cursor.create_error("Build execution is not implemented."_view);
  return {};
}
