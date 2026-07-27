// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/lexical/errors.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/serialization/stream/textual.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Ttx::Lexical;

// The original TTX diagnostic palette gives each part of the message a stable
// visual role: red for the error identity, orange for source context, and
// yellow for the exact range and suggested resolution.
static constexpr View::Bytes clear_color = "\x1b[0m"_view;
static constexpr View::Bytes bold = "\x1b[1m"_view;
static constexpr View::Bytes italic = "\x1b[3m"_view;
static constexpr View::Bytes error_primary = "\x1b[38;2;227;62;60m"_view;
static constexpr View::Bytes error_secondary = "\x1b[38;2;222;122;101m"_view;
static constexpr View::Bytes error_tertiary = "\x1b[38;2;245;147;85m"_view;
static constexpr View::Bytes error_highlight = "\x1b[38;2;255;201;107m"_view;
static constexpr View::Bytes source_color = "\x1b[38;2;255;102;102m"_view;

// Tokens store byte offsets rather than owning source lines. Expand the token
// range to the surrounding line boundaries so every affected line can be
// rendered with its own gutter while still borrowing the stored source text.
static auto source_range(
    View::Bytes source,
    const Token& start_token,
    const Token& end_token) -> View::Bytes {
  Count start = start_token.get_offset();
  Count end = Count(end_token.get_offset()) + end_token.get_size();
  if (start > source.get_size() || end > source.get_size()) {
    return View::Bytes();
  }

  while (start != 0 && source[start - 1] != '\n') {
    start--;
  }
  while (end < source.get_size() && source[end] != '\n') {
    end++;
  }

  return source.slice(start, end - start);
}

// Source gutters reserve five columns for the right-aligned line number. Large
// line numbers are allowed to grow past the gutter instead of being truncated.
static auto decimal_digits(Count value) -> Count {
  Count digits = 1;
  while (value >= 10) {
    value /= 10;
    digits++;
  }
  return digits;
}

static auto write_source_gutter(
    Managed::Bytes& message,
    Stream::Textual<Managed::Bytes>& render,
    Count line) -> void {
  render << error_highlight;
  Count digits = decimal_digits(line);
  if (digits < 5) {
    message.append(' ', 5 - digits);
  }
  render << line << error_tertiary << " | "_view;
}

// The caret belongs to the source excerpt but not to a source line, so it gets
// the same separator with an intentionally empty line-number field.
static auto write_caret_gutter(Stream::Textual<Managed::Bytes>& render)
    -> void {
  render << error_tertiary << "      | "_view << error_highlight;
}

// Renders the following diagnostic shape into the caller's arena (colors are
// omitted here for readability):
//
//   [ERROR] source.ttx:12:5:
//   Explanation of the error
//      12 | first affected source line
//      13 | final affected source line
//         |     ^---------------------
//   Note: optional recovery hint
//
// General errors stop after the explanation because they have no token range.
// An out-of-range index produces an empty view.
auto Errors::render_message(
    Perimortem::Memory::Allocator::Arena& arena,
    Count index) const -> Perimortem::Core::View::Bytes {
  Managed::Bytes message(arena);
  Stream::Textual<Managed::Bytes> render(message);

  if (index >= errors.get_size()) {
    return View::Bytes();
  }

  const Error& error = errors.at(index);
  const Info& source = source_map.at(error.source_id);
  const Bool has_token = error.start_token.is_valid();

  // Stage 1: recover the complete source excerpt and its raw byte span. The
  // underline deliberately covers the same byte range, including any line
  // breaks between the first and last token, as the original renderer did.
  View::Bytes range =
      has_token ? source_range(source.text, error.start_token, error.end_token)
                : View::Bytes();
  Count underline_width = 0;
  Count source_lines = 0;
  if (has_token) {
    Count range_start = error.start_token.get_offset();
    Count range_end =
        Count(error.end_token.get_offset()) + error.end_token.get_size();
    underline_width =
        range_end > range_start ? range_end - range_start : Count(0);
    source_lines =
        error.end_token.get_line() >= error.start_token.get_line()
            ? Count(
                  error.end_token.get_line() - error.start_token.get_line() + 1)
            : Count(1);
  }

  // Stage 2: reserve once before streaming. The fixed per-line allowance
  // covers gutters and color escapes; the remaining terms are emitted bytes.
  message.reset(
      source.name.get_size() + error.message.get_size() +
      error.hint.get_size() + range.get_size() + underline_width +
      (source_lines * 48) + 256);

  // Stage 3: identify the diagnostic and its source location. A general error
  // still names its source, but omits line and column coordinates.
  render << error_primary << bold << "[ERROR] "_view << error_secondary
         << italic << source_color << source.name << ":"_view;
  if (has_token) {
    render << error.start_token.get_line() << ":"_view
           << error.start_token.get_column() << ":"_view;
  }
  render << "\n"_view << clear_color;

  // Stage 4: place the human explanation on its own line before source context.
  render << error_secondary << bold << error.message << "\n"_view;

  if (has_token) {
    // Stage 5: walk the expanded range one line at a time. Keeping line parsing
    // here makes empty lines visible and keeps every gutter aligned.
    Count line_number = error.start_token.get_line();
    Count line_start = 0;
    do {
      Count line_end = line_start;
      while (line_end < range.get_size() && range[line_end] != '\n') {
        line_end++;
      }

      write_source_gutter(message, render, line_number);
      render << range.slice(line_start, line_end - line_start) << "\n"_view;
      line_number++;
      line_start = line_end + 1;
    } while (line_start <= range.get_size());

    // Stage 6: align the caret with the first token column, then preserve the
    // original full byte-range marker even when the range crosses lines.
    write_caret_gutter(render);
    if (error.start_token.get_column() > 1) {
      message.append(' ', error.start_token.get_column() - 1);
    }

    render << "^"_view;
    if (underline_width != 0) {
      message.append('-', underline_width);
    }
    render << "\n"_view;
  }

  if (!error.hint.is_empty()) {
    // Stage 7: finish with the optional recovery hint in its own color pair.
    render << error_tertiary << "Note: "_view << error_highlight << error.hint
           << "\n"_view;
  }

  // Never leak the diagnostic styling into subsequent terminal output.
  render << "\n"_view << clear_color;
  return message;
}
