// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/serialization/binary/stream.hpp"

#include "perimortem/utility/func/data.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Utility::Func;
using namespace Perimortem::Serialization::Binary;

constexpr auto byte_swap_required = __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__;

template <typename storage_type>
constexpr auto possible_swap(storage_type bin) -> storage_type {
  if constexpr (byte_swap_required) {
    switch (sizeof(storage_type)) {
      case 1:
        return bin;
      case 2:
        return __builtin_bswap16(bin);
      case 4:
        return __builtin_bswap32(bin);
      case 8:
        return __builtin_bswap64(bin);
    }
  }

  return bin;
}

template <typename storage_type>
constexpr auto get_data_type() -> Stream::DataType {
  if constexpr (byte_swap_required) {
    switch (sizeof(storage_type)) {
      case 1:
        if constexpr (storage_type(-1) < storage_type(0)) {
          return Stream::DataType::SignedBits_8;
        } else {
          return Stream::DataType::Bits_8;
        }
      case 2:
        if constexpr (storage_type(-1) < storage_type(0)) {
          return Stream::DataType::SignedBits_16;
        } else {
          return Stream::DataType::Bits_16;
        }
      case 4:
        if constexpr (storage_type(-1) < storage_type(0)) {
          return Stream::DataType::SignedBits_32;
        } else {
          return Stream::DataType::Bits_32;
        }
      case 8:
        if constexpr (storage_type(-1) < storage_type(0)) {
          return Stream::DataType::SignedBits_64;
        } else {
          return Stream::DataType::Bits_64;
        }
    }
  }

  return Stream::DataType::Unknown;
}

template <>
constexpr auto get_data_type<Real_32>() -> Stream::DataType {
  return Stream::DataType::Real_32;
}

template <>
constexpr auto get_data_type<Real_64>() -> Stream::DataType {
  return Stream::DataType::Real_64;
}

template <typename blob_type>
constexpr auto write_block(Access::Amorphous target,
                           Count& ptr_location,
                           blob_type bin) -> Bool {
  if (ptr_location + 1 + sizeof(storage_type) > target.get_size()) {
    return false;
  }

  Byte* data = target.get_data();
  data[ptr_location] = static_cast<Byte>(get_data_type<storage_type>());

  bin = possible_swap(bin);

  Data::copy(data + ptr_location + 1, &bin);
  ptr_location += 1 + sizeof(storage_type);

  return true;
}

template <typename storage_type>
constexpr auto write_blob(Access::Amorphous target,
                          Count& ptr_location,
                          storage_type bin) -> Bool {
  if (ptr_location + 1 + sizeof(storage_type) > target.get_size()) {
    return false;
  }

  Byte* data = target.get_data();
  data[ptr_location] = static_cast<Byte>(get_data_type<storage_type>());

  bin = possible_swap(bin);

  Data::copy(data + ptr_location + 1, &bin);
  ptr_location += 1 + sizeof(storage_type);

  return true;
}

auto Stream::set_pointer(Count location) -> void {
  ptr_location = location;
}

auto Stream::operator<<(const Bits_8 bin) -> Stream& {
  write_block(data, ptr_location, bin);
  return *this;
}

auto Stream::operator<<(const Bits_16 bin) -> Stream& {
  write_block(data, ptr_location, bin);
  return *this;
}

auto Stream::operator<<(const Bits_32 bin) -> Stream& {
  write_block(data, ptr_location, bin);
  return *this;
}

auto Stream::operator<<(const Bits_64 bin) -> Stream& {
  write_block(data, ptr_location, bin);
  return *this;
}

auto Stream::operator<<(const SignedBits_8 bin) -> Stream& {
  write_block(data, ptr_location, bin);
  return *this;
}

auto Stream::operator<<(const SignedBits_16 bin) -> Stream& {
  write_block(data, ptr_location, bin);
  return *this;
}

auto Stream::operator<<(const SignedBits_32 bin) -> Stream& {
  write_block(data, ptr_location, bin);
  return *this;
}

auto Stream::operator<<(const SignedBits_64 bin) -> Stream& {
  write_block(data, ptr_location, bin);
  return *this;
}

auto Stream::operator<<(const Real_32 bin) -> Stream& {
  write_block(data, ptr_location, bin);
  return *this;
}

auto Stream::operator<<(const Real_64 bin) -> Stream& {
  write_block(data, ptr_location, bin);
  return *this;
}

auto Stream::operator<<(const Core::View::Amorphous bin) -> Stream& {
  // Write type information
  data.get_data()[ptr_location++] = static_cast<Byte>(DataType::Blob);
  data.get_data()[ptr_location++] = static_cast<Byte>(DataType::Bits_8);

  // Size data
  Count size = possible_swap(bin.get_size());
  Data::copy(data.get_data() + ptr_location, &size);
  ptr_location += sizeof(Count);

  // As a special case we can bulk copy at stride 1 since endianess doesn't come
  // into play.
  Data::copy(data.get_data() + ptr_location, bin.get_data(), bin.get_size());
  ptr_location += bin.get_size();

  return *this;
}
