// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/shader/dialect.hpp"

#include "tetrodotoxin/shader/interpreter/source.hpp"
#include "tetrodotoxin/shader/language/monograph.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

auto Shader::Dialect::interpret(
    Cursor& cursor,
    const Documentation& documentation,
    const Anchor& source_anchor,
    Abstract& context) -> Option<Tetrodotoxin::Language::Monograph&> {
  auto& child = Library::Language::Monograph::create_authored(
      cursor.get_arena(), documentation, source_anchor, library, context);
  auto& monograph = Shader::Language::Monograph::create(
      cursor.get_arena(), *this, documentation, context, child);
  Shader::Interpreter::Source::parse(monograph, cursor);
  return monograph;
}
