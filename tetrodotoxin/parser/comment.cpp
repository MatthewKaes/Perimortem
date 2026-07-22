// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/parser/comment.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "ttx/lexical/lexicon.hpp"
#include "ttx/lexical/token.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Lexical;

namespace Tetrodotoxin::Parser {

static constexpr auto comment_line(Token comment, View::Bytes source)
    -> View::Bytes {
  View::Bytes line = comment.caculate_text(source);
  const Count marker_size =
      Lexicon::get_spelling(Code::Type::Comment).get_size();
  line = line.slice(marker_size, line.get_size() - marker_size);
  if (!line.is_empty() && line[0] == ' ') {
    return line.slice(1, line.get_size() - 1);
  }

  return line;
}

auto Comment::parse(Cursor& cursor)
    -> Option<const Ttx::Model::Documentations::Block&> {
  if (!cursor.matches(Code::Type::Comment)) {
    return none;
  }

  Managed::Vector<View::Bytes> lines(cursor.get_arena());
  while (cursor.matches(Code::Type::Comment)) {
    Token comment;
    View::Bytes line;
    comment = cursor.consume();
    line = comment_line(comment, cursor.get_source_text());
    lines.insert(line);
  }

  // Block and its line index share the Source arena. Each line still borrows
  // the tokenizer's source bytes, avoiding a second prose representation.
  const auto& documentation =
      cursor.get_arena().construct<Ttx::Model::Documentations::Block>(
          lines.get_view());
  return documentation;
}

}  // namespace Tetrodotoxin::Parser
