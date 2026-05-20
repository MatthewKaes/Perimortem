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

template <typename storage_type>
static constexpr auto
    read_block(View::Bytes source, Count& ptr, storage_type& out) -> Bool {
  if (ptr + 1 + sizeof(storage_type) > source.get_size()) [[unlikely]] {
    return False;
  }

  if (static_cast<DataType>(source[ptr++]) != get_data_type<storage_type>())
      [[unlikely]] {
    return False;
  }

  storage_type raw = {};
  memcpy(&raw, source.get_data() + ptr, sizeof(storage_type));
  out = Data::ensure_endian<stream_endian, Data::ByteOrder::Native>(raw);
  ptr += sizeof(storage_type);
  return True;
}

static auto read_blob_bytes(View::Bytes source, Count& ptr, View::Bytes& out)
    -> Bool {
  if (ptr + 3 > source.get_size()) [[unlikely]] {
    return False;
  }

  if (static_cast<DataType>(source[ptr]) != DataType::Blob) [[unlikely]] {
    return False;
  }
  ptr++;

  if (static_cast<DataType>(source[ptr]) != DataType::Bits_8) [[unlikely]] {
    return False;
  }
  ptr++;

  DataType size_type = static_cast<DataType>(source[ptr]);
  ptr++;

  Count blob_size = 0;
  switch (size_type) {
  case DataType::Bits_8: {
    if (ptr + sizeof(Bits_8) > source.get_size()) [[unlikely]] {
      return False;
    }
    blob_size = Count(source[ptr]);
    ptr += sizeof(Bits_8);
    break;
  }
  case DataType::Bits_16: {
    if (ptr + sizeof(Bits_16) > source.get_size()) [[unlikely]] {
      return False;
    }
    Bits_16 raw = 0;
    memcpy(&raw, source.get_data() + ptr, sizeof(Bits_16));
    blob_size =
        Count(Data::ensure_endian<stream_endian, Data::ByteOrder::Native>(raw));
    ptr += sizeof(Bits_16);
    break;
  }
  case DataType::Bits_32: {
    if (ptr + sizeof(Bits_32) > source.get_size()) [[unlikely]] {
      return False;
    }
    Bits_32 raw = 0;
    memcpy(&raw, source.get_data() + ptr, sizeof(Bits_32));
    blob_size =
        Count(Data::ensure_endian<stream_endian, Data::ByteOrder::Native>(raw));
    ptr += sizeof(Bits_32);
    break;
  }
  case DataType::Bits_64: {
    if (ptr + sizeof(Bits_64) > source.get_size()) [[unlikely]] {
      return False;
    }
    Bits_64 raw = 0;
    memcpy(&raw, source.get_data() + ptr, sizeof(Bits_64));
    blob_size =
        Data::ensure_endian<stream_endian, Data::ByteOrder::Native>(raw);
    ptr += sizeof(Bits_64);
    break;
  }
  default:
    return False;
  }

  if (ptr + blob_size > source.get_size()) [[unlikely]] {
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
