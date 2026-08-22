// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/package/language/parser/name.hpp"

#include "ttx/lexical/span.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Package::Language;

// Retains one contiguous authored source span while consuming the selected
// separator and every required Type segment.
static auto parse_qualified_name(
    Cursor& cursor,
    Code::Type separator,
    View::Bytes initial_message,
    View::Bytes segment_message,
    View::Bytes whitespace_message) -> View::Bytes {
  Token first = cursor.require(Code::Type::Type, initial_message);
  if (!first) {
    return {};
  }

  Token last = first;
  while (cursor.matches(separator)) {
    Token operator_token = cursor.current();
    Count previous_end = Count(last.get_offset()) + Count(last.get_size());
    if (operator_token.get_offset() != previous_end) {
      cursor.create_expression_error(
          Span(first, operator_token), whitespace_message);
      return {};
    }

    cursor.consume();
    Token segment = cursor.require(Code::Type::Type, segment_message);
    if (!segment) {
      return {};
    }

    Count operator_end =
        Count(operator_token.get_offset()) + Count(operator_token.get_size());
    if (segment.get_offset() != operator_end) {
      cursor.create_expression_error(Span(first, segment), whitespace_message);
      return {};
    }

    last = segment;
  }

  Count name_start = first.get_offset();
  Count name_end = Count(last.get_offset()) + Count(last.get_size());
  return cursor.get_source_text().slice(name_start, name_end - name_start);
}

auto Parser::Name::parse_semantic(Cursor& cursor) -> Option<Name> {
  View::Bytes spelling = parse_qualified_name(
      cursor, Code::Type::TypeAccessOp,
      "Expected an authored Type shaped semantic name."_view,
      "Semantic name qualification requires a Type segment after `::`."_view,
      "Semantic names cannot contain whitespace around `::`."_view);
  BAIL_IF(spelling.is_empty());
  return Name(spelling);
}

auto Parser::Name::parse_package(Cursor& cursor) -> View::Bytes {
  return parse_qualified_name(
      cursor, Code::Type::AddressOp,
      "Expected an external Type shaped Package name."_view,
      "External Package qualification requires a Type segment after `.`."_view,
      "External Package names cannot contain whitespace around `.`."_view);
}

auto Parser::Name::get_size() const -> Count {
  if (spelling.is_empty()) {
    return 0;
  }

  Count segments = 1;
  for (Count index = 0; index + 1 < spelling.get_size(); index++) {
    if (spelling[index] == ':' && spelling[index + 1] == ':') {
      segments++;
      index++;
    }
  }
  return segments;
}

auto Parser::Name::get_segment(Count requested) const -> View::Bytes {
  Count segment = 0;
  Count start = 0;
  for (Count index = 0; index <= spelling.get_size(); index++) {
    Bool end = index == spelling.get_size();
    Bool separator = !end && index + 1 < spelling.get_size() &&
                     spelling[index] == ':' && spelling[index + 1] == ':';
    if (!end && !separator) {
      continue;
    }
    if (segment == requested) {
      return spelling.slice(start, index - start);
    }
    if (end) {
      return {};
    }
    segment++;
    index++;
    start = index + 1;
  }

  return {};
}
