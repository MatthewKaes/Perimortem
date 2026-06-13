// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/error.hpp"

#include <unistd.h>

#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/writer/textual.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Lexical;

static constexpr auto ansi_reset = "\x1b[0m"_view;
static constexpr auto ansi_bold_red = "\x1b[1;31m"_view;
static constexpr auto ansi_bold = "\x1b[1m"_view;
static constexpr auto ansi_cyan = "\x1b[96m"_view;
static constexpr auto ansi_dim = "\x1b[2m"_view;
static constexpr auto ansi_yellow = "\x1b[33m"_view;

static auto extract_source_line(View::Bytes source, const Token& token)
    -> View::Bytes {
  if (source.empty() || token.get_text().empty()) {
    return View::Bytes();
  }

  const auto* base = source.get_data();
  const auto* start = token.get_text().get_data();
  if (start < base || start >= base + source.get_size()) {
    return View::Bytes();
  }

  Count offset = Count(start - base);
  Count line_start = offset;
  while (line_start > 0 && source[line_start - 1] != '\n') {
    line_start--;
  }

  Count line_end = offset;
  while (line_end < source.get_size() && source[line_end] != '\n') {
    line_end++;
  }

  return source.slice(line_start, line_end - line_start);
}

// Writes the line number into a 4 char wide right-justified gutter followed by
// " | ". This ensures lines stayed aligned for line numbers up to 9999.
static auto write_gutter(Writer::Textual& error_message, Bits_32 line_num)
    -> void {
  error_message << ansi_dim;
  if (line_num < 10) {
    error_message << "   "_view;
  } else if (line_num < 100) {
    error_message << "  "_view;
  } else if (line_num < 1000) {
    error_message << " "_view;
  }

  error_message << line_num << " | "_view;
}

auto Error::report(
    View::Bytes path,
    const Token& token,
    View::Bytes source,
    View::Bytes message,
    View::Bytes hint) -> void {
  // 4 KB should be enough for any resonable error message.
  // If we are overflowing this then we need to seriously rethink the offending
  // error message!
  Static::Bytes<4096> message_buffer;
  Writer::Textual error_message(message_buffer.get_access());

  // error: path.ttx:line:col
  error_message << ansi_bold_red << "error"_view << ansi_reset;
  error_message << ": "_view;
  error_message << ansi_cyan << path << ansi_reset;
  error_message << ":"_view << token.get_line();
  error_message << ":"_view << token.get_column();
  error_message << "\n"_view;

  // Primary message
  error_message << "  "_view << ansi_bold << message << ansi_reset << "\n"_view;

  View::Bytes line = extract_source_line(source, token);
  if (!line.empty()) {
    Bits_32 line_num = token.get_line();
    Bits_32 column = token.get_column();
    Count token_len = Math::max(token.get_text().get_size(), Count(1));

    // Source line with gutter
    error_message << ansi_dim;
    write_gutter(error_message, line_num);
    error_message << ansi_reset << line << "\n"_view;

    // Now output the marking carrot and underline range.
    error_message << "     | "_view;
    error_message << ansi_bold_red;

    for (Bits_32 i = 1; i < column; i++) {
      error_message << " "_view;
    }

    error_message << "^"_view;

    for (Count i = 1; i < token_len; i++) {
      error_message << "~"_view;
    }

    error_message << ansi_reset << "\n"_view;
  }

  if (!hint.empty()) {
    error_message << "  "_view << ansi_yellow << "hint"_view << ansi_reset;
    error_message << ": "_view << hint << "\n"_view;
  }

  error_message << "\n"_view;

  const View::Bytes output(message_buffer);
  write(STDERR_FILENO, output.get_data(), output.get_size());
}
