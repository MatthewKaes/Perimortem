// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/lexical/lexicon.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Lexical;

static auto begins_with(View::Bytes value, View::Bytes prefix) -> Bool {
  return !prefix.is_empty() && prefix.get_size() <= value.get_size() &&
         value.slice(0, prefix.get_size()) == prefix;
}

// Type and Addressable values apply their distinct opening byte before the
// shared continuation character checks.
static auto validate_type(View::Bytes value) -> Bool {
  if (value.is_empty() || value[0] < 'A' || value[0] > 'Z') {
    return False;
  }

  for (Count i = 1; i < value.get_size(); i++) {
    if (!Lexicon::is_type(value[i])) {
      return False;
    }
  }

  return True;
}

static auto validate_addressable(View::Bytes value) -> Bool {
  if (value.is_empty() || value[0] < 'a' || value[0] > 'z') {
    return False;
  }

  for (Count i = 1; i < value.get_size(); i++) {
    if (!Lexicon::is_identifier(value[i])) {
      return False;
    }
  }

  return Lexicon::get_keyword(value, Code::Type::Addressable) ==
         Code::Type::Addressable;
}

// Numeric values contain only decimal digits while Float values contain one
// decimal point after an opening digit.
static auto validate_numeric(View::Bytes value) -> Bool {
  if (value.is_empty()) {
    return False;
  }

  for (Count i = 0; i < value.get_size(); i++) {
    if (!Lexicon::is_numeric(value[i]) || value[i] == '.') {
      return False;
    }
  }

  return True;
}

static auto validate_float(View::Bytes value) -> Bool {
  if (value.is_empty() || value[0] < '0' || value[0] > '9') {
    return False;
  }

  Count points = 0;
  for (Count i = 0; i < value.get_size(); i++) {
    if (!Lexicon::is_numeric(value[i])) {
      return False;
    }

    if (value[i] == '.') {
      points++;
    }
  }

  return points == 1;
}

// Hex values include their fixed prefix and require at least one payload byte.
static auto validate_hex(View::Bytes value) -> Bool {
  View::Bytes prefix = Lexicon::get_spelling(Code::Type::Hex);
  if (!begins_with(value, prefix) || value.get_size() == prefix.get_size()) {
    return False;
  }

  for (Count i = prefix.get_size(); i < value.get_size(); i++) {
    if (!Lexicon::is_hex(value[i])) {
      return False;
    }
  }

  return True;
}

// Range spellings require their closing byte to be the final authored byte.
// An earlier closing byte would have ended the Tokenizer range.
static auto validate_range(
    View::Bytes value,
    Code::Type type,
    Unsigned_8 terminal) -> Bool {
  View::Bytes prefix = Lexicon::get_spelling(type);
  if (!begins_with(value, prefix) || value.get_size() <= prefix.get_size() ||
      value[value.get_size() - 1] != terminal) {
    return False;
  }

  for (Count i = prefix.get_size(); i + 1 < value.get_size(); i++) {
    if (value[i] == terminal) {
      return False;
    }
  }

  return True;
}

// String ranges allow an escaped byte to pass through without interpreting it.
// The final quote must remain unescaped so it closes the complete value.
static auto validate_string(View::Bytes value) -> Bool {
  View::Bytes prefix = Lexicon::get_spelling(Code::Type::String);
  if (!begins_with(value, prefix) || value.get_size() < 2 ||
      value[value.get_size() - 1] != '"') {
    return False;
  }

  for (Count i = prefix.get_size(); i + 1 < value.get_size(); i++) {
    if (value[i] == '\\') {
      i++;
      if (i + 1 >= value.get_size()) {
        return False;
      }
      continue;
    }

    if (value[i] == '"') {
      return False;
    }
  }

  return True;
}

static auto validate_attribute(View::Bytes value) -> Bool {
  View::Bytes prefix = Lexicon::get_spelling(Code::Type::Attribute);
  if (!begins_with(value, prefix)) {
    return False;
  }

  for (Count i = prefix.get_size(); i < value.get_size(); i++) {
    if (!Lexicon::is_identifier(value[i])) {
      return False;
    }
  }

  return True;
}

static auto validate_comment(View::Bytes value) -> Bool {
  View::Bytes prefix = Lexicon::get_spelling(Code::Type::Comment);
  if (!begins_with(value, prefix)) {
    return False;
  }

  for (Count i = prefix.get_size(); i < value.get_size(); i++) {
    if (value[i] == '\n') {
      return False;
    }
  }

  return True;
}

// Applies the one complete spelling rule owned by each supported Code.
static auto validate_code(Code::Type type, View::Bytes value) -> Bool {
  switch (type) {
  case Code::Type::Terminal:
    return value.is_empty();
  case Code::Type::Unknown:
  case Code::Type::PackedData:
    return False;
  case Code::Type::Type:
    return validate_type(value);
  case Code::Type::Addressable:
    return validate_addressable(value);
  case Code::Type::Attribute:
    return validate_attribute(value);
  case Code::Type::Numeric:
    return validate_numeric(value);
  case Code::Type::Float:
    return validate_float(value);
  case Code::Type::Hex:
    return validate_hex(value);
  case Code::Type::String:
    return validate_string(value);
  case Code::Type::Bytes:
    return validate_range(value, type, ']');
  case Code::Type::Embedded:
    return validate_range(value, type, ']');
  case Code::Type::Comment:
    return validate_comment(value);
  default: {
    View::Bytes fixed = Lexicon::get_spelling(type);
    return !fixed.is_empty() && value == fixed;
  }
  }
}

// Returns the longest allowed separator spelling at the current byte so
// overlapping operators follow the same preference as Tokenizer dispatch.
static auto separator_size(
    View::Bytes value,
    Count cursor,
    View::Vector<Code::Type> separators) -> Count {
  Count matched = 0;
  const auto* separator_data = separators.get_data();
  for (Count i = 0; i < separators.get_size(); i++) {
    View::Bytes spelling = Lexicon::get_spelling(separator_data[i]);
    if (spelling.get_size() <= matched ||
        cursor + spelling.get_size() > value.get_size()) {
      continue;
    }

    if (value.slice(cursor, spelling.get_size()) == spelling) {
      matched = spelling.get_size();
    }
  }

  return matched;
}

auto Lexicon::validate(
    Code::Type type,
    View::Bytes value,
    View::Vector<Code::Type> separators) -> Bool {
  if (separators.is_empty()) {
    return validate_code(type, value);
  }

  // Every separator must have one exact fixed spelling before parsing begins.
  const auto* separator_data = separators.get_data();
  for (Count i = 0; i < separators.get_size(); i++) {
    if (get_spelling(separator_data[i]).is_empty()) {
      return False;
    }
  }

  Count segment_start = 0;
  Count cursor = 0;
  while (cursor < value.get_size()) {
    Count matched = separator_size(value, cursor, separators);
    if (matched == 0) {
      cursor++;
      continue;
    }

    if (!validate_code(
            type, value.slice(segment_start, cursor - segment_start))) {
      return False;
    }

    cursor += matched;
    segment_start = cursor;
  }

  return validate_code(type, value.slice(segment_start));
}
