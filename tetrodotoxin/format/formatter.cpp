// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/format/formatter.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Format;

auto Formatter::operator<<(View::Bytes text) -> Formatter& {
  for (Count i = 0; i < text.get_size(); i++) {
    Bits_8 ch = text[i];
    out.append(ch);

    if (ch == '\n') {
      trailing_newlines++;
      column = 0;
    } else {
      trailing_newlines = 0;
      column++;
    }
  }

  return *this;
}

auto Formatter::emit_newline() -> void {
  out.append('\n');
  trailing_newlines++;
  column = 0;
}

auto Formatter::ensure_blank() -> void {
  while (trailing_newlines < 2) {
    out.append('\n');
    trailing_newlines++;
  }
}

auto Formatter::do_indent() -> void {
  for (Count i = 0; i < indent; i++) {
    out.concat("  "_view);
    column += 2;
  }

  trailing_newlines = 0;
}

auto Formatter::emit_padding(Count target, Count current) -> void {
  if (target <= current) {
    return;
  }

  for (Count i = current; i < target; i++) {
    out.append(' ');
    column++;
  }

  trailing_newlines = 0;
}

auto Formatter::current_line_width() const -> Count {
  return column;
}
