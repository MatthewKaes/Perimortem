// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/writer/serial.hpp"

#include "perimortem/core/data.hpp"

using namespace Perimortem::Core;

constexpr auto native_endian = Data::ByteOrder::Native;
constexpr auto stream_endian = Data::ByteOrder::Little;

constexpr auto negate_flag = 0x10;
constexpr auto blob_flag = 0x20;

constexpr auto write_value(
    Access::Bytes target,
    Count& cursor,
    Signed_64 value,
    Unsigned_8 flags) -> Bool {
  // Negate any negative values and store the operation as a flag.
  if (value < -1) {
    // Store negate flag
    flags |= negate_flag;
    value = -value;
  }

  // Choose the smallest integer type that fits the element count.
  Count encoding_size = 8;
  if (value <= Unsigned_8(-1)) {
    encoding_size = 1;
  } else if (value <= Unsigned_16(-1)) {
    encoding_size = 2;
  } else if (value <= Unsigned_32(-1)) {
    encoding_size = 4;
  }

  if (cursor + 1 + encoding_size > target.get_size()) [[unlikely]] {
    return False;
  }

  auto data = target.get_data();
  data[cursor++] = static_cast<Unsigned_8>(encoding_size | flags);
  switch (encoding_size) {
  case 1:
    data[cursor] = Unsigned_8(value);
    break;
  case 2:
    Data::copy(
        data + cursor,
        Data::ensure_endian<native_endian, stream_endian, Unsigned_16>(value));
    break;
  case 4:
    Data::copy(
        data + cursor,
        Data::ensure_endian<native_endian, stream_endian, Unsigned_32>(value));
    break;
  case 8:
    Data::copy(
        data + cursor,
        Data::ensure_endian<native_endian, stream_endian, Unsigned_64>(value));
    break;
  }

  cursor += encoding_size;
  return True;
}

constexpr auto write_blob(
    Access::Bytes target,
    Count& cursor,
    View::Bytes source) -> Bool {
  if (!write_value(target, cursor, source.get_size(), blob_flag)) {
    return False;
  }

  if (cursor + source.get_size() > target.get_size()) {
    return False;
  }

  auto data = target.get_data();
  Data::copy(data + cursor, source.get_data(), source.get_size());
  cursor += source.get_size();
  return True;
}

auto Writer::Serial::set_pointer(Count location) -> void {
  cursor = location;
}

auto Writer::Serial::operator<<(const Unsigned_64 value) -> Writer::Serial& {
  valid_state &= write_value(source, cursor, value, 0x00);
  return *this;
}

auto Writer::Serial::operator<<(const View::Bytes blob) -> Writer::Serial& {
  // Write type information
  valid_state &= write_blob(source, cursor, blob);
  return *this;
}
