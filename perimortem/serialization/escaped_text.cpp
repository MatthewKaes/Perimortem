// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/serialization/escaped_text.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "perimortem/memory/managed/bytes.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;

static auto hex_value(Bits_8 value) -> Signed_32 {
  if (value >= '0' && value <= '9') {
    return value - '0';
  }

  if (value >= 'A' && value <= 'F') {
    return value - 'A' + 10;
  }

  if (value >= 'a' && value <= 'f') {
    return value - 'a' + 10;
  }

  return -1;
}

static auto read_hex_quad(View::Bytes source, Count position, Bits_32& value)
    -> Bool {
  if (position + 4 > source.get_size()) {
    return False;
  }

  value = 0;
  for (Count i = 0; i < 4; i++) {
    Signed_32 digit = hex_value(source[position + i]);
    if (digit < 0) {
      return False;
    }

    value = (value << 4) | Bits_32(digit);
  }

  return True;
}

static auto is_high_surrogate(Bits_32 value) -> Bool {
  return value >= 0xD800 && value <= 0xDBFF;
}

static auto is_low_surrogate(Bits_32 value) -> Bool {
  return value >= 0xDC00 && value <= 0xDFFF;
}

static auto append_utf8(Managed::Bytes& output, Bits_32 codepoint) -> void {
  if (codepoint <= 0x7F) {
    output.append(Bits_8(codepoint));
    return;
  }

  if (codepoint <= 0x7FF) {
    output.append(Bits_8(0xC0 | (codepoint >> 6)));
    output.append(Bits_8(0x80 | (codepoint & 0x3F)));
    return;
  }

  if (codepoint <= 0xFFFF) {
    output.append(Bits_8(0xE0 | (codepoint >> 12)));
    output.append(Bits_8(0x80 | ((codepoint >> 6) & 0x3F)));
    output.append(Bits_8(0x80 | (codepoint & 0x3F)));
    return;
  }

  if (codepoint <= 0x10FFFF) {
    output.append(Bits_8(0xF0 | (codepoint >> 18)));
    output.append(Bits_8(0x80 | ((codepoint >> 12) & 0x3F)));
    output.append(Bits_8(0x80 | ((codepoint >> 6) & 0x3F)));
    output.append(Bits_8(0x80 | (codepoint & 0x3F)));
  }
}

static auto read_unicode_escape(
    View::Bytes source,
    Count slash,
    Bits_32& codepoint,
    Count& consumed) -> Bool {
  if (slash + 6 > source.get_size() || source[slash] != '\\' ||
      source[slash + 1] != 'u') {
    return False;
  }

  Bits_32 first = 0;
  if (!read_hex_quad(source, slash + 2, first)) {
    return False;
  }

  if (!is_high_surrogate(first)) {
    if (is_low_surrogate(first)) {
      return False;
    }

    codepoint = first;
    consumed = 6;
    return True;
  }

  if (slash + 12 > source.get_size() || source[slash + 6] != '\\' ||
      source[slash + 7] != 'u') {
    return False;
  }

  Bits_32 second = 0;
  if (!read_hex_quad(source, slash + 8, second) || !is_low_surrogate(second)) {
    return False;
  }

  codepoint = 0x10000 + ((first - 0xD800) << 10) + (second - 0xDC00);
  consumed = 12;
  return True;
}

static auto contains_escape_sequence(View::Bytes source) -> Bool {
  for (Count i = 0; i < source.get_size(); i++) {
    if (source[i] == '\\') {
      return True;
    }
  }

  return False;
}

static auto json_escape_size(Bits_8 value) -> Count {
  switch (value) {
  case '"':
  case '\\':
  case '\b':
  case '\f':
  case '\n':
  case '\r':
  case '\t':
    return 2;
  default:
    return value < 0x20 ? Count(6) : Count(1);
  }
}

static auto requires_json_escape(Bits_8 value) -> Bool {
  return value == '"' || value == '\\' || value < 0x20;
}

static auto scan_json_escape(View::Bytes source, Count position) -> Count {
  for (; position < source.get_size(); position++) {
    if (requires_json_escape(source[position])) {
      return position;
    }
  }

  return Count(-1);
}

static auto append_json_escape(Writer::Textual& output, Bits_8 value) -> void {
  switch (value) {
  case '"':
    output << '\\';
    output << '"';
    return;
  case '\\':
    output << '\\';
    output << '\\';
    return;
  case '\b':
    output << '\\';
    output << 'b';
    return;
  case '\f':
    output << '\\';
    output << 'f';
    return;
  case '\n':
    output << '\\';
    output << 'n';
    return;
  case '\r':
    output << '\\';
    output << 'r';
    return;
  case '\t':
    output << '\\';
    output << 't';
    return;
  default:
    break;
  }

  if (value >= 0x20) {
    output << Signed_8(value);
    return;
  }

  constexpr Static::Vector<Bits_8, 16> hex_digits = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
  output << '\\';
  output << 'u';
  output << '0';
  output << '0';
  output << Signed_8(hex_digits[value >> 4]);
  output << Signed_8(hex_digits[value & 0x0F]);
}

auto EscapedText::decode(Allocator::Arena& arena, View::Bytes source, Style)
    -> View::Bytes {
  if (!contains_escape_sequence(source)) {
    return source;
  }

  Managed::Bytes decoded(arena);

  for (Count i = 0; i < source.get_size(); i++) {
    if (source[i] != '\\' || i + 1 >= source.get_size()) {
      decoded.append(source[i]);
      continue;
    }

    Count slash = i;
    i++;
    switch (source[i]) {
    case 'n':
      decoded.append('\n');
      break;
    case 'r':
      decoded.append('\r');
      break;
    case 't':
      decoded.append('\t');
      break;
    case '"':
      decoded.append('"');
      break;
    case '\\':
      decoded.append('\\');
      break;
    case '/':
      decoded.append('/');
      break;
    case 'b':
      decoded.append('\b');
      break;
    case 'f':
      decoded.append('\f');
      break;
    case 'u': {
      Bits_32 codepoint = 0;
      Count consumed = 0;
      if (read_unicode_escape(source, slash, codepoint, consumed)) {
        append_utf8(decoded, codepoint);
        i = slash + consumed - 1;
        break;
      }

      decoded.append('\\');
      decoded.append(source[i]);
      break;
    }
    default:
      decoded.append('\\');
      decoded.append(source[i]);
      break;
    }
  }

  return decoded.get_view();
}

auto EscapedText::encoded_size(View::Bytes source, Style) -> Count {
  Count result = 0;
  for (Count i = 0; i < source.get_size(); i++) {
    result += json_escape_size(source[i]);
  }

  return result;
}

auto EscapedText::encode(Writer::Textual& output, View::Bytes source, Style)
    -> void {
  Count clean_start = 0;
  while (clean_start < source.get_size()) {
    Count escape = scan_json_escape(source, clean_start);
    if (escape == Count(-1)) {
      output << source.slice(clean_start);
      return;
    }

    if (escape > clean_start) {
      output << source.slice(clean_start, escape - clean_start);
    }

    append_json_escape(output, source[escape]);
    clean_start = escape + 1;
  }
}
