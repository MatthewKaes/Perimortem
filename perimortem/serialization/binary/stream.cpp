// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/serialization/binary/stream.hpp"

#include "perimortem/core/data.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Serialization;

constexpr auto stream_endian = Data::ByteOrder::Little;

template <typename storage_type>
constexpr auto get_data_type() -> Binary::Stream::DataType {
  switch (sizeof(storage_type)) {
    case 1:
      if constexpr (storage_type(-1) < storage_type(0)) {
        return Binary::Stream::DataType::SignedBits_8;
      } else {
        return Binary::Stream::DataType::Bits_8;
      }
    case 2:
      if constexpr (storage_type(-1) < storage_type(0)) {
        return Binary::Stream::DataType::SignedBits_16;
      } else {
        return Binary::Stream::DataType::Bits_16;
      }
    case 4:
      if constexpr (storage_type(-1) < storage_type(0)) {
        return Binary::Stream::DataType::SignedBits_32;
      } else {
        return Binary::Stream::DataType::Bits_32;
      }
    case 8:
      if constexpr (storage_type(-1) < storage_type(0)) {
        return Binary::Stream::DataType::SignedBits_64;
      } else {
        return Binary::Stream::DataType::Bits_64;
      }
  }

  return Binary::Stream::DataType::Unknown;
}

template <>
constexpr auto get_data_type<Real_32>() -> Binary::Stream::DataType {
  return Binary::Stream::DataType::Real_32;
}

template <>
constexpr auto get_data_type<Real_64>() -> Binary::Stream::DataType {
  return Binary::Stream::DataType::Real_64;
}

template <typename storage_type>
constexpr auto write_block(Access::Bytes target,
                           Count& ptr_location,
                           storage_type bin) -> Bool {
  if (ptr_location + 1 + sizeof(storage_type) > target.get_size()) {
    return false;
  }

  Byte* data = target.get_data();
  data[ptr_location] = static_cast<Byte>(get_data_type<storage_type>());

  bin = Data::ensure_endian<Data::ByteOrder::Native, stream_endian>(bin);

  Data::copy(data + ptr_location + 1, &bin);
  ptr_location += 1 + sizeof(storage_type);

  return true;
}

template <typename blob_type>
constexpr auto write_blob(Access::Bytes target,
                          Count& ptr_location,
                          blob_type bin) -> Bool {
  using storage_type = typename blob_type::data_type;
  if (ptr_location + 2 + sizeof(storage_type) * bin.get_size() >
      target.get_size()) {
    return false;
  }

  Byte* data = target.get_data();
  data[ptr_location++] = static_cast<Byte>(Binary::Stream::DataType::Blob);
  data[ptr_location++] = static_cast<Byte>(get_data_type<storage_type>());

  // Size data
  Count size = Data::ensure_endian<Data::ByteOrder::Native, stream_endian>(bin.get_size());
  Data::copy(data + ptr_location, &size);
  ptr_location += sizeof(Count);

  // Special case for byte data
  if constexpr (sizeof(storage_type) == 1 ||
                (Data::ByteOrder::Native == stream_endian)) {
    Data::copy(data + ptr_location, bin.get_data(), bin.get_size());
    ptr_location += sizeof(storage_type) * bin.get_size();
  } else {
    // Serialize each block
    for (Count i = 0; i < bin.get_size(); i++) {
      storage_type ordered_bytes =
          Data::ensure_endian<Data::ByteOrder::Native, stream_endian>(
              bin.get_data()[i]);
      Data::copy(data + ptr_location, &ordered_bytes);
      ptr_location += sizeof(storage_type);
    }
  }

  return true;
}

auto Binary::Stream::set_pointer(Count location) -> void {
  ptr_location = location;
}

auto Binary::Stream::operator<<(const Bits_8 bin) -> Binary::Stream& {
  valid_state &= write_block(data, ptr_location, bin);
  return *this;
}

auto Binary::Stream::operator<<(const Bits_16 bin) -> Binary::Stream& {
  valid_state &= write_block(data, ptr_location, bin);
  return *this;
}

auto Binary::Stream::operator<<(const Bits_32 bin) -> Binary::Stream& {
  valid_state &= write_block(data, ptr_location, bin);
  return *this;
}

auto Binary::Stream::operator<<(const Bits_64 bin) -> Binary::Stream& {
  valid_state &= write_block(data, ptr_location, bin);
  return *this;
}

auto Binary::Stream::operator<<(const SignedBits_8 bin) -> Binary::Stream& {
  valid_state &= write_block(data, ptr_location, bin);
  return *this;
}

auto Binary::Stream::operator<<(const SignedBits_16 bin) -> Binary::Stream& {
  valid_state &= write_block(data, ptr_location, bin);
  return *this;
}

auto Binary::Stream::operator<<(const SignedBits_32 bin) -> Binary::Stream& {
  valid_state &= write_block(data, ptr_location, bin);
  return *this;
}

auto Binary::Stream::operator<<(const SignedBits_64 bin) -> Binary::Stream& {
  valid_state &= write_block(data, ptr_location, bin);
  return *this;
}

auto Binary::Stream::operator<<(const Real_32 bin) -> Binary::Stream& {
  valid_state &= write_block(data, ptr_location, bin);
  return *this;
}

auto Binary::Stream::operator<<(const Real_64 bin) -> Binary::Stream& {
  valid_state &= write_block(data, ptr_location, bin);
  return *this;
}

auto Binary::Stream::operator<<(const View::Bytes blob) -> Binary::Stream& {
  // Write type information
  valid_state &= write_blob(data, ptr_location, blob);
  return *this;
}

auto Binary::Stream::operator<<(const View::Vector<Bits_8> blob)
    -> Binary::Stream& {
  // Write type information
  valid_state &= write_blob(data, ptr_location, blob);
  return *this;
}

auto Binary::Stream::operator<<(const View::Vector<Bits_16> blob)
    -> Binary::Stream& {
  // Write type information
  valid_state &= write_blob(data, ptr_location, blob);
  return *this;
}

auto Binary::Stream::operator<<(const View::Vector<Bits_32> blob)
    -> Binary::Stream& {
  // Write type information
  valid_state &= write_blob(data, ptr_location, blob);
  return *this;
}

auto Binary::Stream::operator<<(const View::Vector<Bits_64> blob)
    -> Binary::Stream& {
  // Write type information
  valid_state &= write_blob(data, ptr_location, blob);
  return *this;
}

auto Binary::Stream::operator<<(const View::Vector<SignedBits_8> blob)
    -> Binary::Stream& {
  // Write type information
  valid_state &= write_blob(data, ptr_location, blob);
  return *this;
}

auto Binary::Stream::operator<<(const View::Vector<SignedBits_16> blob)
    -> Binary::Stream& {
  // Write type information
  valid_state &= write_blob(data, ptr_location, blob);
  return *this;
}

auto Binary::Stream::operator<<(const View::Vector<SignedBits_32> blob)
    -> Binary::Stream& {
  // Write type information
  valid_state &= write_blob(data, ptr_location, blob);
  return *this;
}

auto Binary::Stream::operator<<(const View::Vector<SignedBits_64> blob)
    -> Binary::Stream& {
  // Write type information
  valid_state &= write_blob(data, ptr_location, blob);
  return *this;
}
