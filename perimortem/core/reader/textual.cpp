// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/reader/textual.hpp"

#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;

template <typename storage_type>
auto parse_decimal(View::Bytes source, Count& cursor) -> storage_type {
  if (cursor >= source.get_size()) [[unlikely]] {
    return storage_type();
  }

  storage_type sign = 1;
  if constexpr (storage_type(0) > storage_type(-1)) {
    if (source[cursor] == '-') {
      sign = -1;
      cursor++;
    }
  }

  Bits_8 first = source[cursor];
  if (first < '0' || first > '9') [[unlikely]] {
    cursor = Count(-1);
    return storage_type();
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

  return result * sign;
}

auto skip_whitespace(View::Bytes source, Count& cursor) -> void {
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
  if (!has_content()) [[unlikely]] {
    cursor = Count(-1);
    return Bits_8(0);
  }

  return source.get_data()[cursor++];
}

auto Reader::Textual::read_flag() -> Bool {
  skip_whitespace(source, cursor);
  if (!has_content()) [[unlikely]] {
    cursor = Count(-1);
    return false;
  }

  // Use a switch for cheap case insensitivity.
  switch (source.get_data()[cursor]) {
  case 'T':
  case 't':
    if (source.slice(cursor + 1, 3) == "rue"_view) {
      cursor += 4;
      return true;
    } else {
      cursor = Count(-1);
      return false;
    }

  case 'F':
  case 'f':
    if (source.slice(cursor + 1, 4) == "alse"_view) {
      cursor += 5;
    } else {
      cursor = Count(-1);
    }

    return false;

  default:
    cursor = Count(-1);
    return false;
  }
}

auto Reader::Textual::read_unsigned() -> Bits_64 {
  skip_whitespace(source, cursor);
  return parse_decimal<Bits_64>(source, cursor);
}

auto Reader::Textual::read_signed() -> Signed_64 {
  skip_whitespace(source, cursor);
  return parse_decimal<Signed_64>(source, cursor);
}

auto Reader::Textual::read_real_32() -> Real_32 {
  return Real_32(read_real_64());
}

auto Reader::Textual::read_real_64() -> Real_64 {
  skip_whitespace(source, cursor);
  if (!has_content()) [[unlikely]] {
    cursor = Count(-1);
    return Real_64(0);
  }

  // Get the raw data to avoid extra range checks.
  Real_64 sign = 1;
  if (source[cursor] == '-') {
    sign = -1;
    cursor++;
  }

  Real_64 result = Real_64(parse_decimal<Bits_64>(source, cursor));
  if (!has_content()) [[unlikely]] {
    return result * sign;
  }

  // Check if we have a floating point portion at all.
  auto data = source.get_data();
  if (data[cursor] != '.') {
    return result * sign;
  }

  Real_64 frac_mult = 0.1;
  cursor++;
  while (has_content()) {
    Bits_8 character = data[cursor];
    if (character < '0' || character > '9') {
      break;
    }

    result += Real_64(character - '0') * frac_mult;
    frac_mult *= 0.1;
    cursor++;
  }

  return result * sign;
}
