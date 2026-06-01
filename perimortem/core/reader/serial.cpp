// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/reader/serial.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/writer/textual.hpp"

using namespace Perimortem::Core;

constexpr auto native_endian = Data::ByteOrder::Native;
constexpr auto stream_endian = Data::ByteOrder::Little;

constexpr auto negate_flag = 0x10;
constexpr auto blob_flag = 0x20;

auto Reader::Serial::set_pointer(Count location) -> void {
  ptr_location = location < data.get_size() ? location : data.get_size();
}

auto Reader::Serial::read() -> Value {
  if (!valid_state || ptr_location >= data.get_size()) {
    Static::Bytes<128> error_buffer;
    Writer::Textual error_message(error_buffer);
    error_message
        << "Serial read overran data buffer while reading type at byte location "_view
        << ptr_location << ". source_size="_view << data.get_size();
    Diagnostics::Log::error(error_message);

    // Set to invalid
    valid_state = False;
    return Value();
  }

  auto source = data.get_data();
  auto byte_flag = source[ptr_location++];
  const auto encoded_size = byte_flag & 0xF;

  if (ptr_location + encoded_size > data.get_size()) {
    Static::Bytes<128> error_buffer;
    Writer::Textual error_message(error_buffer);
    error_message
        << "Serial read overran data buffer while reading value at byte location "_view
        << ptr_location << ". source_size="_view << data.get_size()
        << " encoded_size="_view << encoded_size;
    Diagnostics::Log::error(error_message);

    // Set to invalid
    valid_state = False;
    return Value();
  }

  SignedBits_64 value;
  switch (encoded_size) {
  case 1:
    value = source[ptr_location];
    break;
  case 2: {
    Bits_16 actual_bytes;
    memcpy(&actual_bytes, source + ptr_location, sizeof(Bits_16));
    value = Data::ensure_endian<stream_endian, native_endian>(actual_bytes);
    break;
  }
  case 4: {
    Bits_32 actual_bytes;
    memcpy(&actual_bytes, source + ptr_location, sizeof(Bits_32));
    value = Data::ensure_endian<stream_endian, native_endian>(actual_bytes);
    break;
  }
  case 8:
    memcpy(&value, source + ptr_location, sizeof(Bits_64));
    value = Data::ensure_endian<stream_endian, native_endian>(value);
    break;
  default: {
    Static::Bytes<128> error_buffer;
    Writer::Textual error_message(error_buffer);
    error_message << "Serial read found invalid encoding size "_view
                  << (encoded_size) << " at byte location "_view << ptr_location
                  << "."_view;
    Diagnostics::Log::error(error_message);

    // Set to invalid
    valid_state = False;
    return Value();
  }
  }

  // Bump pointer by size
  ptr_location += encoded_size;

  // Take the twos complement of the value if the negate flag is set.
  if (byte_flag & negate_flag) {
    value = -value;
  }

  // If it's not a blob then we can return the value directly.
  if (!(byte_flag & blob_flag)) {
    return Value(value);
  }

  if (ptr_location + value > data.get_size()) {
    Static::Bytes<128> error_buffer;
    Writer::Textual error_message(error_buffer);
    error_message << "Serial read of blob sized "_view << value
                  << " bytes overran source buffer at location "_view
                  << ptr_location << ". source_size="_view << data.get_size()
                  << " blob_size="_view << value;
    Diagnostics::Log::error(error_message);

    // Set to invalid
    valid_state = False;
    return Value();
  }

  // Bump the pointer to include the blob as well.
  auto blob_ptr = source + ptr_location;
  ptr_location += value;
  return View::Bytes(blob_ptr, value);
}

auto Reader::Serial::read_value() -> SignedBits_64 {
  // If attribution isn't set then make it explicit any failures were from the
  // read_value call rather than just read.
  auto attribution = Diagnostics::Log::set_attribution();
  return read().get_value();
}

auto Reader::Serial::read_blob() -> View::Bytes {
  // If attribution isn't set then make it explicit any failures were from the
  // read_blob call rather than just read.
  auto attribution = Diagnostics::Log::set_attribution();
  return read().get_view();
}
