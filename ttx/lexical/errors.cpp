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

Errors::Report::Report(
    Errors& errors,
    View::Bytes source_name,
    View::Bytes source_text,
    Span span)
    : errors(errors),
      source_name(source_name),
      source_text(source_text),
      span(span),
      message_storage(errors.arena),
      hint_storage(errors.arena),
      message(message_storage),
      hint(hint_storage) {}

Errors::Report::~Report() {
  if (message_storage.get_size() == 0) {
    return;
  }

  errors.publish_report(
      source_name, source_text, message_storage, hint_storage, span);
}

auto Errors::retain_source(View::Bytes source_name, View::Bytes source_text)
    -> View::Bytes {
  auto* retained = source_map.find(source_name);
  if (retained != nullptr) {
    return retained->key;
  }

  View::Bytes retained_name = arena.proxy(source_name);
  View::Bytes retained_text = arena.proxy(source_text);
  source_map.insert(retained_name, retained_text);
  return retained_name;
}

auto Errors::publish_report(
    View::Bytes source_name,
    View::Bytes source_text,
    View::Bytes message,
    View::Bytes hint,
    Span span) -> void {
  // Report message storage already belongs to this Arena. Retain or recover the
  // canonical source name without copying a source body already retained here.
  errors.insert({
    .message = message,
    .hint = hint,
    .source_name = retain_source(source_name, source_text),
    .span = span,
  });
}

// Tokens store byte offsets rather than owning source lines. Expand the token
// range to the surrounding line boundaries so every affected line can be
// rendered with its own gutter while still borrowing the stored source text.
static auto source_range(View::Bytes source, Span span) -> View::Bytes {
  // A general report carries no authored coordinates. Keep its excerpt empty
  // instead of reading the unspecified coordinates of an invalid Span.
  if (!span) {
    return View::Bytes();
  }

  Count span_start = span.get_offset();
  Count span_end = span_start + span.get_size();
  if (span_start > source.get_size() || span_end > source.get_size()) {
    return View::Bytes();
  }

  // The opening Token may sit in the middle of a line. Recover the preceding
  // bytes so the first gutter still presents the complete authored line.
  Count start = span.get_offset();
  while (start != 0 && source[start - 1] != '\n') {
    start--;
  }

  // The closing Token may also sit before the line ending. Retaining the
  // trailing bytes gives the final affected line the same complete context.
  Count end = span.get_offset() + span.get_size();
  while (end < source.get_size() && source[end] != '\n') {
    end++;
  }

  return source.slice(start, end - start);
}

// Source gutters reserve five columns for the line number aligned on the
// right. Large line numbers grow past the gutter instead of being truncated.
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
// the same separator with an intentionally empty line number field.
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
//         |     ^ plus its underline
//   Note: optional recovery hint
//
// General errors stop after the explanation because they have no Span.
// An index outside the retained reports produces an empty view.
auto Errors::render_message(
    Perimortem::Memory::Allocator::Arena& arena,
    Count index) const -> Perimortem::Core::View::Bytes {
  Managed::Bytes message(arena);
  Stream::Textual<Managed::Bytes> render(message);

  if (index >= errors.get_size()) {
    return View::Bytes();
  }

  const Error& error = errors.at(index);
  const auto* source = source_map.find(error.source_name);
  if (source == nullptr) {
    return View::Bytes();
  }

  View::Bytes source_name = source->key;
  View::Bytes source_text = source->value;
  const Span token_span = error.span;
  const Count span_size = token_span ? token_span.get_size() : 0;
  const Count span_line_count = token_span ? token_span.get_line_count() : 0;

  // The excerpt expands to full source lines while the underline retains the
  // exact Span width, including line breaks between its Tokens.
  View::Bytes range = source_range(source_text, token_span);

  // One reservation covers the borrowed text plus gutters and color escapes.
  // General reports contribute zero Span bytes without reading coordinates.
  message.reset(
      source_name.get_size() + error.message.get_size() +
      error.hint.get_size() + range.get_size() + span_size +
      (span_line_count * 48) + 256);

  // A general report still names its source, but its invalid Span deliberately
  // omits line and column coordinates.
  render << error_primary << bold << "[ERROR] "_view << error_secondary
         << italic << source_color << source_name << ":"_view;
  if (token_span) {
    render << token_span.get_line() << ":"_view << token_span.get_column()
           << ":"_view;
  }
  render << "\n"_view << clear_color;

  render << error_secondary << bold << error.message << "\n"_view;

  if (token_span) {
    // Span retains coordinates rather than line slices. Splitting the expanded
    // range here keeps empty lines visible and every gutter aligned.
    Count line_number = token_span.get_line();
    Count line_start = 0;
    for (Count i = 0; i <= range.get_size(); i++) {
      if (i != range.get_size() && range[i] != '\n') {
        continue;
      }

      // A source range excludes its final newline, so reaching the View end is
      // the only reliable way to publish its last character or empty line.
      write_source_gutter(message, render, line_number);
      render << range.slice(line_start, i - line_start) << "\n"_view;
      line_number++;
      line_start = i + 1;
    }

    // The caret begins at the opening Token column. Its fill remains the exact
    // byte width even when the Span crosses line boundaries.
    write_caret_gutter(render);
    if (token_span.get_column() > 1) {
      message.append(' ', token_span.get_column() - 1);
    }

    // The caret already marks the first byte. One fewer fill byte makes the
    // visible marker exactly as wide as the Span.
    render << "^"_view;
    if (span_size > 1) {
      message.append('-', span_size - 1);
    }
    render << "\n"_view;
  }

  if (!error.hint.is_empty()) {
    render << error_tertiary << "Note: "_view << error_highlight << error.hint
           << "\n"_view;
  }

  // Never leak the diagnostic styling into subsequent terminal output.
  render << "\n"_view << clear_color;
  return message;
}
