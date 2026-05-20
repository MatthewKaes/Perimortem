// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/reader/textual.hpp"

#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;

static auto parse_unsigned(View::Bytes source, Count& ptr, ULong& out) -> Bool {
  if (ptr >= source.get_size()) [[unlikely]] {
    return False;
  }

  Byte first = source[ptr];
  if (first < '0' || first > '9') [[unlikely]] {
    return False;
  }

  ULong result = 0;
  while (ptr < source.get_size()) {
    Byte ch = source[ptr];
    if (ch < '0' || ch > '9') {
      break;
    }
    result = result * 10 + ULong(ch - '0');
    ptr++;
  }

  out = result;
  return True;
}

static auto parse_signed(View::Bytes source, Count& ptr, Long& out) -> Bool {
  if (ptr >= source.get_size()) [[unlikely]] {
    return False;
  }

  Bool negative = False;
  if (source[ptr] == '-') {
    negative = True;
    ptr++;
  }

  ULong magnitude = 0;
  if (!parse_unsigned(source, ptr, magnitude)) [[unlikely]] {
    return False;
  }

  out = negative ? -Long(magnitude) : Long(magnitude);
  return True;
}

auto Reader::Textual::set_pointer(Count location) -> void {
  ptr_location = location < data.get_size() ? location : data.get_size();
}

auto Reader::Textual::skip_whitespace() -> void {
  while (ptr_location < data.get_size()) {
    Byte ch = data[ptr_location];
    if (ch != ' ' && ch != '\n' && ch != '\r' && ch != '\t') {
      break;
    }
    ptr_location++;
  }
}

auto Reader::Textual::read_byte() -> Byte {
  if (!valid_state) [[unlikely]] {
    return Byte(0);
  }
  if (ptr_location >= data.get_size()) [[unlikely]] {
    valid_state = False;
    return Byte(0);
  }
  return data[ptr_location++];
}

auto Reader::Textual::read_boolean() -> Bool {
  if (!valid_state) [[unlikely]] {
    return False;
  }
  skip_whitespace();

  constexpr Count true_len = 4;
  constexpr Count false_len = 5;

  if (ptr_location + true_len <= data.get_size()) {
    if (data.slice(ptr_location, true_len) == "true"_view) {
      ptr_location += true_len;
      return True;
    }
  }

  if (ptr_location + false_len <= data.get_size()) {
    if (data.slice(ptr_location, false_len) == "false"_view) {
      ptr_location += false_len;
      return False;
    }
  }

  valid_state = False;
  return False;
}

auto Reader::Textual::read_int16() -> Half {
  skip_whitespace();
  Long value = 0;
  valid_state &= parse_signed(data, ptr_location, value);
  return Half(value);
}

auto Reader::Textual::read_uint16() -> UHalf {
  skip_whitespace();
  ULong value = 0;
  valid_state &= parse_unsigned(data, ptr_location, value);
  return UHalf(value);
}

auto Reader::Textual::read_int32() -> Int {
  skip_whitespace();
  Long value = 0;
  valid_state &= parse_signed(data, ptr_location, value);
  return Int(value);
}

auto Reader::Textual::read_uint32() -> UInt {
  skip_whitespace();
  ULong value = 0;
  valid_state &= parse_unsigned(data, ptr_location, value);
  return UInt(value);
}

auto Reader::Textual::read_int64() -> Long {
  skip_whitespace();
  Long value = 0;
  valid_state &= parse_signed(data, ptr_location, value);
  return value;
}

auto Reader::Textual::read_uint64() -> ULong {
  skip_whitespace();
  ULong value = 0;
  valid_state &= parse_unsigned(data, ptr_location, value);
  return value;
}

auto Reader::Textual::read_real_32() -> Real_32 {
  skip_whitespace();
  return Real_32(read_real());
}

auto Reader::Textual::read_real_64() -> Real_64 {
  skip_whitespace();
  return read_real();
}

auto Reader::Textual::read_real() -> Real_64 {
  if (!valid_state) [[unlikely]] {
    return Real_64(0);
  }
  if (ptr_location >= data.get_size()) [[unlikely]] {
    valid_state = False;
    return Real_64(0);
  }

  Bool negative = False;
  if (data[ptr_location] == '-') {
    negative = True;
    ptr_location++;
  }

  ULong int_part = 0;
  if (!parse_unsigned(data, ptr_location, int_part)) [[unlikely]] {
    valid_state = False;
    return Real_64(0);
  }

  Real_64 result = Real_64(int_part);

  if (ptr_location < data.get_size() && data[ptr_location] == '.') {
    ptr_location++;

    Real_64 frac_mult = 0.1;
    while (ptr_location < data.get_size()) {
      Byte ch = data[ptr_location];
      if (ch < '0' || ch > '9') {
        break;
      }
      result += Real_64(ch - '0') * frac_mult;
      frac_mult *= 0.1;
      ptr_location++;
    }
  }

  return negative ? -result : result;
}
