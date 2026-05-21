// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/reader/serial.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/writer/textual.hpp"

using namespace Perimortem::Core;

constexpr auto stream_endian = Data::ByteOrder::Little;
using DataType = Writer::Serial::DataType;

template <typename storage_type>
constexpr auto get_data_type() -> DataType {
  switch (sizeof(storage_type)) {
  case 1:
    if constexpr (storage_type(-1) < storage_type(0)) {
      return DataType::SignedBits_8;
    } else {
      return DataType::Bits_8;
    }
  case 2:
    if constexpr (storage_type(-1) < storage_type(0)) {
      return DataType::SignedBits_16;
    } else {
      return DataType::Bits_16;
    }
  case 4:
    if constexpr (storage_type(-1) < storage_type(0)) {
      return DataType::SignedBits_32;
    } else {
      return DataType::Bits_32;
    }
  case 8:
    if constexpr (storage_type(-1) < storage_type(0)) {
      return DataType::SignedBits_64;
    } else {
      return DataType::Bits_64;
    }
  }

  return DataType::Unknown;
}

template <>
constexpr auto get_data_type<Real_32>() -> DataType {
  return DataType::Real_32;
}

template <>
constexpr auto get_data_type<Real_64>() -> DataType {
  return DataType::Real_64;
}

static auto check_buffer_overruns(Count ptr, Count source_size, Count read_size)
    -> Bool {
  if (ptr + read_size > source_size) [[unlikely]] {
    Static::Bytes<128> error_buffer;
    Writer::Textual error_message(error_buffer);
    error_message << "Serial read over ran buffer at read location "_view << ptr
                  << ". source_size="_view << source_size << ", read_size="_view
                  << read_size;
    Diagnostics::Log::error(error_message);
    return False;
  }

  return True;
}

static auto check_type(View::Bytes source, Count& ptr, DataType required_type)
    -> Bool {
  if (static_cast<DataType>(source[ptr++]) != required_type) [[unlikely]] {
    Static::Bytes<128> error_buffer;
    Writer::Textual error_message(error_buffer);
    error_message
        << "Mismatched Data Type in serilization stream at position "_view
        << ptr << ". Expected="_view << Int(required_type) << ", Got="_view
        << Int(source[ptr - 1]);
    Diagnostics::Log::error(error_message);

    return False;
  }

  return True;
}

template <typename storage_type>
static constexpr auto
    read_block(View::Bytes source, Count& ptr, storage_type& out) -> Bool {
  constexpr auto read_size = 1 + sizeof(storage_type);
  if (!check_buffer_overruns(ptr, source.get_size(), read_size)) {
    return False;
  }

  if (!check_type(source, ptr, get_data_type<storage_type>())) {
    return False;
  }

  storage_type raw = {};
  memcpy(&raw, source.get_data() + ptr, sizeof(storage_type));
  out = Data::ensure_endian<stream_endian, Data::ByteOrder::Native>(raw);
  ptr += sizeof(storage_type);
  return True;
}

template <typename size_type>
static auto read_run_lenght_size(View::Bytes source, Count& ptr) -> Count {
  if (!check_buffer_overruns(ptr, source.get_size(), sizeof(size_type))) {
    return Count(-1);
  }

  size_type blob_size = 0;
  memcpy(&blob_size, source.get_data() + ptr, sizeof(size_type));
  ptr += sizeof(size_type);
  return Count(
      Data::ensure_endian<stream_endian, Data::ByteOrder::Native>(blob_size));
}

static auto read_blob_bytes(View::Bytes source, Count& ptr, View::Bytes& out)
    -> Bool {
  // Need to be able to read at least the header.
  if (!check_buffer_overruns(ptr, source.get_size(), 3)) {
    return False;
  }

  if (!check_type(source, ptr, DataType::Blob)) {
    return False;
  }

  if (!check_type(source, ptr, DataType::Bits_8)) {
    return False;
  }

  DataType size_type = static_cast<DataType>(source[ptr]);
  ptr++;

  Count blob_size = 0;
  switch (size_type) {
  case DataType::Bits_8: {
    blob_size = read_run_lenght_size<Bits_8>(source, ptr);
    break;
  }
  case DataType::Bits_16: {
    blob_size = read_run_lenght_size<Bits_16>(source, ptr);
    break;
  }
  case DataType::Bits_32: {
    blob_size = read_run_lenght_size<Bits_32>(source, ptr);
    break;
  }
  case DataType::Bits_64: {
    blob_size = read_run_lenght_size<Bits_64>(source, ptr);
  }
  default: {
    Static::Bytes<128> error_buffer;
    Writer::Textual error_message(error_buffer);
    error_message << "Serial blob read encountered invalid size encoding type. "
                     "Expected=245|246|247|248, Got="_view
                  << Int(size_type);
    Diagnostics::Log::error(error_message);
    return False;
  }
  }

  if (!check_buffer_overruns(ptr, source.get_size(), blob_size)) {
    return False;
  }

  out = source.slice(ptr, blob_size);
  ptr += blob_size;
  return True;
}

auto Reader::Serial::set_pointer(Count location) -> void {
  ptr_location = location < data.get_size() ? location : data.get_size();
}

auto Reader::Serial::read_bits_8() -> Bits_8 {
  Bits_8 out = 0;
  valid_state &= read_block(data, ptr_location, out);
  return out;
}

auto Reader::Serial::read_bits_16() -> Bits_16 {
  Bits_16 out = 0;
  valid_state &= read_block(data, ptr_location, out);
  return out;
}

auto Reader::Serial::read_bits_32() -> Bits_32 {
  Bits_32 out = 0;
  valid_state &= read_block(data, ptr_location, out);
  return out;
}

auto Reader::Serial::read_bits_64() -> Bits_64 {
  Bits_64 out = 0;
  valid_state &= read_block(data, ptr_location, out);
  return out;
}

auto Reader::Serial::read_signed_bits_8() -> SignedBits_8 {
  SignedBits_8 out = 0;
  valid_state &= read_block(data, ptr_location, out);
  return out;
}

auto Reader::Serial::read_signed_bits_16() -> SignedBits_16 {
  SignedBits_16 out = 0;
  valid_state &= read_block(data, ptr_location, out);
  return out;
}

auto Reader::Serial::read_signed_bits_32() -> SignedBits_32 {
  SignedBits_32 out = 0;
  valid_state &= read_block(data, ptr_location, out);
  return out;
}

auto Reader::Serial::read_signed_bits_64() -> SignedBits_64 {
  SignedBits_64 out = 0;
  valid_state &= read_block(data, ptr_location, out);
  return out;
}

auto Reader::Serial::read_real_32() -> Real_32 {
  Real_32 out = Real_32(0);
  valid_state &= read_block(data, ptr_location, out);
  return out;
}

auto Reader::Serial::read_real_64() -> Real_64 {
  Real_64 out = Real_64(0);
  valid_state &= read_block(data, ptr_location, out);
  return out;
}

auto Reader::Serial::read_blob() -> View::Bytes {
  View::Bytes out;
  valid_state &= read_blob_bytes(data, ptr_location, out);
  return out;
}
