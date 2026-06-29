// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/format/syntax.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Format;
using namespace Tetrodotoxin::Lexical;

auto Tetrodotoxin::Format::format(Formatter& ctx, const Ast::Comment& comment)
    -> void {
  View::Vector<View::Bytes> lines = comment.get_lines();

  for (Count i = 0; i < lines.get_size(); i++) {
    ctx.do_indent();

    // TODO: clang format only splits comments, but TTX should join multiple
    //       comments in a row together then split them on line length
    //       boundaries so users never need to think about fixing comments.
    //       Lines that start with multiple spaces aren't merged and empty `//`
    //       comment lines are perserved and treated as delimiters between
    //       comment blocks.
    ctx << Class::Type::Comment;
    if (!lines[i].is_empty()) {
      ctx << " "_view;
      ctx << lines[i];
    }

    ctx.emit_newline();
  }
}
