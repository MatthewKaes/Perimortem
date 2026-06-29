// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/error.hpp"

#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/writer/textual.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Core::Diagnostics;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Lexical;

constexpr auto clear_color = "\x1b[0m"_view;
constexpr auto dark_color = "\x1b[38;5;238m"_view;
constexpr auto error_color = "\x1b[38;5;160m"_view;
constexpr auto message_color = "\x1b[1m\x1b[38;5;210m"_view;
constexpr auto resolve_color = "\x1b[38;5;214m"_view;
constexpr auto resolution_color = "\x1b[38;5;228m"_view;

static auto color(View::Bytes value, Bool use_color) -> View::Bytes {
  return use_color ? value : View::Bytes();
}

static auto extract_source_line(View::Bytes source, const Token& token)
    -> View::Bytes {
  if (source.is_empty() || token.get_text().is_empty()) {
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
static auto write_gutter(
    Writer::Textual& error_message,
    Signed_32 line,
    Bool use_color) -> void {
  error_message << color(dark_color, use_color);

  // Empty line case
  if (line < 0) {
    error_message << "     | "_view << color(clear_color, use_color);
    return;
  }

  // Fixed width gutters
  if (line < 10) {
    error_message << "   "_view;
  } else if (line < 100) {
    error_message << "  "_view;
  } else if (line < 1000) {
    error_message << " "_view;
  }

  error_message << line << " | "_view << color(clear_color, use_color);
}

static auto range_width(const Token& start, const Token& end, View::Bytes line)
    -> Count {
  if (start.get_line() != end.get_line()) {
    return line.get_size() - (start.get_column() - 1);
  }

  Count start_column = start.get_column();
  Count end_column = end.get_column() + end.get_text().get_size();

  if (end_column <= start_column) {
    return start.get_text().get_size();
  }

  return end_column - start_column;
}

auto Error::render(
    Access::Bytes target,
    View::Bytes,
    const Token& start,
    const Token& end,
    View::Bytes source,
    View::Bytes message,
    View::Bytes hint,
    Bool use_color) -> Count {
  Writer::Textual error_message(target);
  // Print out the error level (currently only error). The diagnostics logger
  // owns file, line, and column formatting through Diagnostics::Source.
  error_message << color(error_color, use_color) << "error: "_view;
  error_message << color(message_color, use_color) << message << '\n'
                << color(clear_color, use_color);

  // TODO: errors in the middle of a line should get previous lines as well for
  //       context (like in packs).
  View::Bytes line = extract_source_line(source, start);
  if (!line.is_empty()) {
    // Source line with gutter
    write_gutter(error_message, start.get_line(), use_color);
    error_message << line << '\n';

    // Now output the marking carrot and underline range.
    write_gutter(error_message, -1, use_color);
    error_message << color(error_color, use_color);

    // Carrot padding
    for (Bits_32 i = 1; i < start.get_column(); i++) {
      error_message << ' ';
    }

    // Offending token carrot
    error_message << '^';

    // Error range indicator.
    Count underline_width = range_width(start, end, line);
    for (Count i = 1; i < underline_width; i++) {
      error_message << '~';
    }

    error_message << color(clear_color, use_color) << '\n';
  }

  // If the error provided a resolution hint then forward that to the user.
  if (!hint.is_empty()) {
    error_message << color(resolve_color, use_color) << "Note: "_view
                  << color(resolution_color, use_color) << hint << '\n';
  }

  // Padding error message.
  error_message << '\n';

  return error_message.get_location();
}

auto Error::render(
    View::Bytes path,
    const Token& start,
    const Token& end,
    View::Bytes source,
    View::Bytes message,
    View::Bytes hint,
    Bool use_color) -> Perimortem::Memory::Dynamic::Bytes {
  // 4 KB should be enough for any reasonable error message.
  // If we are overflowing this then we need to seriously rethink the offending
  // error message!
  Static::Bytes<Error::max_render_size> message_buffer;
  Count written = render(
      message_buffer.get_access(), path, start, end, source, message, hint,
      use_color);

  const View::Bytes output(message_buffer.get_data(), written);
  return Perimortem::Memory::Dynamic::Bytes(output);
}

auto Error::report(
    View::Bytes path,
    const Token& token,
    View::Bytes source,
    View::Bytes message,
    View::Bytes hint) -> void {
  report(path, token, token, source, message, hint);
}

auto Error::report(
    View::Bytes path,
    const Token& start,
    const Token& end,
    View::Bytes source,
    View::Bytes message,
    View::Bytes hint) -> void {
  Perimortem::Memory::Dynamic::Bytes output =
      render(path, start, end, source, message, hint);
  Log::error(
      output.get_view(), Source(path, start.get_line(), start.get_column()));
}
