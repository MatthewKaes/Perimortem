// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/writer/binary.hpp"

#include "perimortem/core/data.hpp"

using namespace Perimortem::Core;

// write_value and write_vector are local template helpers so the member
// implementations can stay in this .cpp file without leaking into the header.

template <Data::ByteOrder endian, typename storage_type>
static constexpr auto
    write_value(Access::Bytes target, Count& ptr, storage_type val) -> Bool {
  Count base = Count(target.get_data());
  ptr = Data::align<sizeof(storage_type)>(base + ptr) - base;

  if (ptr + sizeof(storage_type) > target.get_size()) [[unlikely]] {
    return False;
  }

  val = Data::ensure_endian<Data::ByteOrder::Native, endian>(val);
  Data::copy(target.get_data() + ptr, val);
  ptr += sizeof(storage_type);
  return True;
}

// memcpy is actually too smart for it's own good and will try to interpret
// bytes for floats and reals rather than just loading it as is.
template <Data::ByteOrder endian, typename storage_type, typename real_type>
static constexpr auto
    write_real(Access::Bytes target, Count& ptr, real_type val)
        -> Bool {  // Make unit tests fail at least on mismatch.
  if constexpr (sizeof(storage_type) != sizeof(real_type)) {
    return False;
  }

  Count base = Count(target.get_data());
  ptr = Data::align<sizeof(storage_type)>(base + ptr) - base;

  if (ptr + sizeof(storage_type) > target.get_size()) [[unlikely]] {
    return False;
  }

  storage_type actual_bytes = *Data::cast<storage_type>(&val);
  actual_bytes =
      Data::ensure_endian<Data::ByteOrder::Native, endian>(actual_bytes);
  Data::copy(target.get_data() + ptr, actual_bytes);
  ptr += sizeof(storage_type);
  return True;
}

template <Data::ByteOrder endian, typename blob_type>
static constexpr auto
    write_vector(Access::Bytes target, Count& ptr, blob_type blob) -> Bool {
  using element_type = typename blob_type::data_type;
  Count base = Count(target.get_data());
  ptr = Data::align<sizeof(element_type)>(base + ptr) - base;

  Count total = sizeof(element_type) * blob.get_size();
  if (ptr + total > target.get_size()) [[unlikely]] {
    return False;
  }

  // Optimize for View::Bytes and when writing in the native endianness.
  if constexpr (
      sizeof(element_type) == 1 || Data::ByteOrder::Native == endian) {
    Data::copy(target.get_data() + ptr, blob.get_data(), blob.get_size());
    ptr += total;
  } else {
    for (Count index = 0; index < blob.get_size(); index++) {
      element_type element =
          Data::ensure_endian<Data::ByteOrder::Native, endian>(
              blob.get_data()[index]);
      Data::copy(target.get_data() + ptr, &element);
      ptr += sizeof(element_type);
    }
  }
  return True;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::set_pointer(Count location) -> void {
  ptr_location = location < data.get_size() ? location : data.get_size();
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const Bits_8 bin) -> Binary& {
  valid_state &= write_value<stream_endian>(data, ptr_location, bin);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const Bits_16 bin) -> Binary& {
  valid_state &= write_value<stream_endian>(data, ptr_location, bin);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const Bits_32 bin) -> Binary& {
  valid_state &= write_value<stream_endian>(data, ptr_location, bin);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const Bits_64 bin) -> Binary& {
  valid_state &= write_value<stream_endian>(data, ptr_location, bin);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const Signed_8 bin) -> Binary& {
  valid_state &= write_value<stream_endian>(data, ptr_location, bin);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const Signed_16 bin) -> Binary& {
  valid_state &= write_value<stream_endian>(data, ptr_location, bin);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const Signed_32 bin) -> Binary& {
  valid_state &= write_value<stream_endian>(data, ptr_location, bin);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const Signed_64 bin) -> Binary& {
  valid_state &= write_value<stream_endian>(data, ptr_location, bin);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const Real_32 bin) -> Binary& {
  valid_state &= write_real<stream_endian, Bits_32>(data, ptr_location, bin);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const Real_64 bin) -> Binary& {
  valid_state &= write_real<stream_endian, Bits_64>(data, ptr_location, bin);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const View::Bytes raw)
    -> Binary& {
  if (ptr_location + raw.get_size() > data.get_size()) [[unlikely]] {
    valid_state = False;
    return *this;
  }
  Data::copy(data.get_data() + ptr_location, raw.get_data(), raw.get_size());
  ptr_location += raw.get_size();
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const View::Vector<Bits_8> blob)
    -> Binary& {
  valid_state &= write_vector<stream_endian>(data, ptr_location, blob);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const View::Vector<Bits_16> blob)
    -> Binary& {
  valid_state &= write_vector<stream_endian>(data, ptr_location, blob);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const View::Vector<Bits_32> blob)
    -> Binary& {
  valid_state &= write_vector<stream_endian>(data, ptr_location, blob);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const View::Vector<Bits_64> blob)
    -> Binary& {
  valid_state &= write_vector<stream_endian>(data, ptr_location, blob);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(
    const View::Vector<Signed_8> blob) -> Binary& {
  valid_state &= write_vector<stream_endian>(data, ptr_location, blob);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(
    const View::Vector<Signed_16> blob) -> Binary& {
  valid_state &= write_vector<stream_endian>(data, ptr_location, blob);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(
    const View::Vector<Signed_32> blob) -> Binary& {
  valid_state &= write_vector<stream_endian>(data, ptr_location, blob);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(
    const View::Vector<Signed_64> blob) -> Binary& {
  valid_state &= write_vector<stream_endian>(data, ptr_location, blob);
  return *this;
}

template class Writer::Binary<Data::ByteOrder::Little>;
template class Writer::Binary<Data::ByteOrder::Big>;
