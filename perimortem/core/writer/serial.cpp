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
    Count& ptr_location,
    Signed_64 value,
    Bits_8 flags) -> Bool {
  // Negate any negative values and store the operation as a flag.
  if (value < -1) {
    // Store negate flag
    flags |= negate_flag;
    value = -value;
  }

  // Choose the smallest integer type that fits the element count.
  Count encoding_size = 8;
  if (value <= Bits_8(-1)) {
    encoding_size = 1;
  } else if (value <= Bits_16(-1)) {
    encoding_size = 2;
  } else if (value <= Bits_32(-1)) {
    encoding_size = 4;
  }

  if (ptr_location + 1 + encoding_size > target.get_size()) [[unlikely]] {
    return False;
  }

  auto data = target.get_data();
  data[ptr_location++] = static_cast<Bits_8>(encoding_size | flags);
  switch (encoding_size) {
  case 1:
    data[ptr_location] = Bits_8(value);
    break;
  case 2:
    Data::copy(
        data + ptr_location,
        Data::ensure_endian<native_endian, stream_endian, Bits_16>(value));
    break;
  case 4:
    Data::copy(
        data + ptr_location,
        Data::ensure_endian<native_endian, stream_endian, Bits_32>(value));
    break;
  case 8:
    Data::copy(
        data + ptr_location,
        Data::ensure_endian<native_endian, stream_endian, Bits_64>(value));
    break;
  }
  ptr_location += encoding_size;

  return True;
}

constexpr auto write_blob(
    Access::Bytes target,
    Count& ptr_location,
    View::Bytes source) -> Bool {
  if (!write_value(target, ptr_location, source.get_size(), blob_flag)) {
    return False;
  }

  if (ptr_location + source.get_size() > target.get_size()) {
    return False;
  }

  Data::copy(
      target.get_data() + ptr_location, source.get_data(), source.get_size());
  ptr_location += source.get_size();
  return True;
}

auto Writer::Serial::set_pointer(Count location) -> void {
  ptr_location = location;
}

auto Writer::Serial::operator<<(const Bits_64 bin) -> Writer::Serial& {
  valid_state &= write_value(data, ptr_location, bin, 0x00);
  return *this;
}

auto Writer::Serial::operator<<(const View::Bytes blob) -> Writer::Serial& {
  // Write type information
  valid_state &= write_blob(data, ptr_location, blob);
  return *this;
}
