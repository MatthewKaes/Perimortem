// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/reader/textual.hpp"

#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;

template <typename storage_type>
auto parse_decimal(
    View::Bytes source,
    Count& cursor,
    storage_type& output_value) -> Bool {
  if (cursor >= source.get_size()) [[unlikely]] {
    return False;
  }

  Bool negative = False;
  if constexpr (storage_type(0) > storage_type(-1)) {
    if (source[cursor] == '-') {
      negative = True;
      cursor++;
    }
  }

  Bits_8 first = source[cursor];
  if (first < '0' || first > '9') [[unlikely]] {
    return False;
  }

  Bits_64 result = 0;
  while (cursor < source.get_size()) {
    Bits_8 character = source[cursor];
    if (character < '0' || character > '9') {
      break;
    }
    result = result * 10 + Bits_64(character - '0');
    cursor++;
  }

  output_value = negative ? -storage_type(result) : storage_type(result);
  return True;
}

auto Reader::Textual::set_pointer(Count location) -> void {
  cursor = location < source.get_size() ? location : source.get_size();
}

auto Reader::Textual::skip_whitespace() -> void {
  // Outer loop already does a bounds check so grab the raw pointer.
  auto text = source.get_data();
  while (cursor < source.get_size()) {
    Bits_8 value = text[cursor];
    if (value != ' ' && value != '\n' && value != '\r' && value != '\t') {
      break;
    }
    cursor++;
  }
}

auto Reader::Textual::read_byte() -> Bits_8 {
  if (!valid_state) [[unlikely]] {
    return Bits_8(0);
  }
  if (cursor >= source.get_size()) [[unlikely]] {
    valid_state = False;
    return Bits_8(0);
  }
  return source.get_data()[cursor++];
}

auto Reader::Textual::read_flag() -> Bool {
  skip_whitespace();
  if (cursor >= source.get_size()) [[unlikely]] {
    valid_state = False;
    return False;
  }

  switch (source.get_data()[cursor]) {
  case 'T':
  case 't':
    valid_state = source.slice(cursor + 1, "rue"_view.get_size()) == "rue"_view;
    cursor += 4;
    return True;

  case 'F':
  case 'f':
    valid_state =
        source.slice(cursor + 1, "alse"_view.get_size()) == "alse"_view;
    cursor += 5;
    return False;

  default:
    valid_state = False;
    return False;
  }
}

auto Reader::Textual::read_unsigned() -> Bits_64 {
  skip_whitespace();
  Bits_64 value = 0;
  valid_state &= parse_decimal(source, cursor, value);
  return value;
}

auto Reader::Textual::read_signed() -> Signed_64 {
  skip_whitespace();
  Signed_64 value = 0;
  valid_state &= parse_decimal(source, cursor, value);
  return value;
}

auto Reader::Textual::read_real_32() -> Real_32 {
  return Real_32(read_real_64());
}

auto Reader::Textual::read_real_64() -> Real_64 {
  skip_whitespace();

  auto data = source.get_data();

  if (!valid_state) [[unlikely]] {
    return Real_64(0);
  }
  if (cursor >= source.get_size()) [[unlikely]] {
    valid_state = False;
    return Real_64(0);
  }

  Bool negative = False;
  if (data[cursor] == '-') {
    negative = True;
    cursor++;
  }

  Signed_64 int_part = 0;
  if (!parse_decimal(source, cursor, int_part)) [[unlikely]] {
    valid_state = False;
    return Real_64(0);
  }

  Real_64 result = Real_64(int_part);

  if (cursor < source.get_size() && data[cursor] == '.') {
    cursor++;

    Real_64 frac_mult = 0.1;
    while (cursor < source.get_size()) {
      Bits_8 character = data[cursor];
      if (character < '0' || character > '9') {
        break;
      }
      result += Real_64(character - '0') * frac_mult;
      frac_mult *= 0.1;
      cursor++;
    }
  }

  return negative ? -result : result;
}
