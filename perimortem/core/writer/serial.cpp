// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/writer/serial.hpp"

#include "perimortem/core/data.hpp"

using namespace Perimortem::Core;

constexpr auto stream_endian = Data::ByteOrder::Little;

template <typename storage_type>
constexpr auto get_data_type() -> Writer::Serial::DataType {
  switch (sizeof(storage_type)) {
  case 1:
    if constexpr (storage_type(-1) < storage_type(0)) {
      return Writer::Serial::DataType::SignedBits_8;
    } else {
      return Writer::Serial::DataType::Bits_8;
    }
  case 2:
    if constexpr (storage_type(-1) < storage_type(0)) {
      return Writer::Serial::DataType::SignedBits_16;
    } else {
      return Writer::Serial::DataType::Bits_16;
    }
  case 4:
    if constexpr (storage_type(-1) < storage_type(0)) {
      return Writer::Serial::DataType::SignedBits_32;
    } else {
      return Writer::Serial::DataType::Bits_32;
    }
  case 8:
    if constexpr (storage_type(-1) < storage_type(0)) {
      return Writer::Serial::DataType::SignedBits_64;
    } else {
      return Writer::Serial::DataType::Bits_64;
    }
  }

  return Writer::Serial::DataType::Unknown;
}

template <>
constexpr auto get_data_type<Real_32>() -> Writer::Serial::DataType {
  return Writer::Serial::DataType::Real_32;
}

template <>
constexpr auto get_data_type<Real_64>() -> Writer::Serial::DataType {
  return Writer::Serial::DataType::Real_64;
}

template <typename storage_type>
constexpr auto
    write_block(Access::Bytes target, Count& ptr_location, storage_type bin)
        -> Bool {
  if (ptr_location + 1 + sizeof(storage_type) > target.get_size())
      [[unlikely]] {
    return False;
  }

  Byte* data = target.get_data();
  data[ptr_location] = static_cast<Byte>(get_data_type<storage_type>());

  bin = Data::ensure_endian<Data::ByteOrder::Native, stream_endian>(bin);

  Data::copy(data + ptr_location + 1, &bin);
  ptr_location += 1 + sizeof(storage_type);

  return True;
}

template <typename blob_type>
constexpr auto
    write_blob(Access::Bytes target, Count& ptr_location, blob_type bin)
        -> Bool {
  using storage_type = typename blob_type::data_type;
  Count blob_count = bin.get_size();

  // Choose the smallest integer type that fits the element count.
  Count size_type_bytes =
      blob_count <= Count(Bits_8(-1))   ? sizeof(Bits_8)
      : blob_count <= Count(Bits_16(-1)) ? sizeof(Bits_16)
      : blob_count <= Count(Bits_32(-1)) ? sizeof(Bits_32)
                                         : sizeof(Bits_64);

  if (ptr_location + 3 + size_type_bytes + sizeof(storage_type) * blob_count >
      target.get_size()) [[unlikely]] {
    return False;
  }

  Byte* data = target.get_data();
  data[ptr_location++] = static_cast<Byte>(Writer::Serial::DataType::Blob);
  data[ptr_location++] = static_cast<Byte>(get_data_type<storage_type>());

  // Write size using the selected encoding.
  if (blob_count <= Count(Bits_8(-1))) {
    data[ptr_location++] =
        static_cast<Byte>(Writer::Serial::DataType::Bits_8);
    Bits_8 sized_count = Bits_8(blob_count);
    Data::copy(data + ptr_location, &sized_count);
    ptr_location += sizeof(Bits_8);
  } else if (blob_count <= Count(Bits_16(-1))) {
    data[ptr_location++] =
        static_cast<Byte>(Writer::Serial::DataType::Bits_16);
    Bits_16 sized_count =
        Data::ensure_endian<Data::ByteOrder::Native, stream_endian>(
            Bits_16(blob_count));
    Data::copy(data + ptr_location, &sized_count);
    ptr_location += sizeof(Bits_16);
  } else if (blob_count <= Count(Bits_32(-1))) {
    data[ptr_location++] =
        static_cast<Byte>(Writer::Serial::DataType::Bits_32);
    Bits_32 sized_count =
        Data::ensure_endian<Data::ByteOrder::Native, stream_endian>(
            Bits_32(blob_count));
    Data::copy(data + ptr_location, &sized_count);
    ptr_location += sizeof(Bits_32);
  } else {
    data[ptr_location++] =
        static_cast<Byte>(Writer::Serial::DataType::Bits_64);
    Bits_64 sized_count =
        Data::ensure_endian<Data::ByteOrder::Native, stream_endian>(
            blob_count);
    Data::copy(data + ptr_location, &sized_count);
    ptr_location += sizeof(Bits_64);
  }

  if constexpr (
      sizeof(storage_type) == 1 || Data::ByteOrder::Native == stream_endian) {
    Data::copy(data + ptr_location, bin.get_data(), bin.get_size());
    ptr_location += sizeof(storage_type) * bin.get_size();
  } else {
    for (Count index = 0; index < bin.get_size(); index++) {
      storage_type ordered_bytes =
          Data::ensure_endian<Data::ByteOrder::Native, stream_endian>(
              bin.get_data()[index]);
      Data::copy(data + ptr_location, &ordered_bytes);
      ptr_location += sizeof(storage_type);
    }
  }

  return True;
}

auto Writer::Serial::set_pointer(Count location) -> void {
  ptr_location = location;
}

auto Writer::Serial::operator<<(const Bits_8 bin) -> Writer::Serial& {
  valid_state &= write_block(data, ptr_location, bin);
  return *this;
}

auto Writer::Serial::operator<<(const Bits_16 bin) -> Writer::Serial& {
  valid_state &= write_block(data, ptr_location, bin);
  return *this;
}

auto Writer::Serial::operator<<(const Bits_32 bin) -> Writer::Serial& {
  valid_state &= write_block(data, ptr_location, bin);
  return *this;
}

auto Writer::Serial::operator<<(const Bits_64 bin) -> Writer::Serial& {
  valid_state &= write_block(data, ptr_location, bin);
  return *this;
}

auto Writer::Serial::operator<<(const SignedBits_8 bin) -> Writer::Serial& {
  valid_state &= write_block(data, ptr_location, bin);
  return *this;
}

auto Writer::Serial::operator<<(const SignedBits_16 bin) -> Writer::Serial& {
  valid_state &= write_block(data, ptr_location, bin);
  return *this;
}

auto Writer::Serial::operator<<(const SignedBits_32 bin) -> Writer::Serial& {
  valid_state &= write_block(data, ptr_location, bin);
  return *this;
}

auto Writer::Serial::operator<<(const SignedBits_64 bin) -> Writer::Serial& {
  valid_state &= write_block(data, ptr_location, bin);
  return *this;
}

auto Writer::Serial::operator<<(const Real_32 bin) -> Writer::Serial& {
  valid_state &= write_block(data, ptr_location, bin);
  return *this;
}

auto Writer::Serial::operator<<(const Real_64 bin) -> Writer::Serial& {
  valid_state &= write_block(data, ptr_location, bin);
  return *this;
}

auto Writer::Serial::operator<<(const View::Bytes blob) -> Writer::Serial& {
  // Write type information
  valid_state &= write_blob(data, ptr_location, blob);
  return *this;
}

auto Writer::Serial::operator<<(const View::Vector<Bits_8> blob)
    -> Writer::Serial& {
  // Write type information
  valid_state &= write_blob(data, ptr_location, blob);
  return *this;
}

auto Writer::Serial::operator<<(const View::Vector<Bits_16> blob)
    -> Writer::Serial& {
  // Write type information
  valid_state &= write_blob(data, ptr_location, blob);
  return *this;
}

auto Writer::Serial::operator<<(const View::Vector<Bits_32> blob)
    -> Writer::Serial& {
  // Write type information
  valid_state &= write_blob(data, ptr_location, blob);
  return *this;
}

auto Writer::Serial::operator<<(const View::Vector<Bits_64> blob)
    -> Writer::Serial& {
  // Write type information
  valid_state &= write_blob(data, ptr_location, blob);
  return *this;
}

auto Writer::Serial::operator<<(const View::Vector<SignedBits_8> blob)
    -> Writer::Serial& {
  // Write type information
  valid_state &= write_blob(data, ptr_location, blob);
  return *this;
}

auto Writer::Serial::operator<<(const View::Vector<SignedBits_16> blob)
    -> Writer::Serial& {
  // Write type information
  valid_state &= write_blob(data, ptr_location, blob);
  return *this;
}

auto Writer::Serial::operator<<(const View::Vector<SignedBits_32> blob)
    -> Writer::Serial& {
  // Write type information
  valid_state &= write_blob(data, ptr_location, blob);
  return *this;
}

auto Writer::Serial::operator<<(const View::Vector<SignedBits_64> blob)
    -> Writer::Serial& {
  // Write type information
  valid_state &= write_blob(data, ptr_location, blob);
  return *this;
}
