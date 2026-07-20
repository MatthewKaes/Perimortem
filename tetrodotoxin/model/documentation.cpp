// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/documentation.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "ttx/lexical/lexicon.hpp"
#include "ttx/model/documentations/block.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Lexical;

auto Tetrodotoxin::Model::Documentation::evaluate(Cursor& cursor)
    -> const Ttx::Concept::Documentation& {
  Managed::Vector<View::Bytes> lines(cursor.get_arena());

  // Keep authored line order while removing only the lexical comment marker
  // and its optional presentation space. All remaining text is semantic
  // Documentation owned by the produced definition.
  while (cursor.matches(Code::Type::Comment)) {
    const Token comment = cursor.consume();
    View::Bytes line = comment.caculate_text(cursor.get_source_text());
    const Count header_size =
        Lexicon::get_spelling(Code::Type::Comment).get_size();
    line = line.slice(header_size, line.get_size() - header_size);
    if (line[0] == ' ') {
      line = line.slice(1, line.get_size() - 1);
    }
    lines.insert(line);
  }

  if (lines.is_empty()) {
    return Ttx::Concept::Documentation::get_empty();
  }

  // Block borrows the normalized lines from the same Source arena that owns
  // their bytes, so no second text representation is created.
  return cursor.get_arena().construct<Ttx::Model::Documentations::Block>(
      lines.get_view());
}
