// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/writer/binary.hpp"

#include "perimortem/core/data.hpp"

using namespace Perimortem::Core;

// write_value and write_vector are local template helpers so the member
// implementations can stay in this .cpp file without leaking into the header.

template <Data::ByteOrder endian, typename storage_type>
static constexpr auto
    write_value(Access::Bytes target, Count& cursor, storage_type value)
        -> Bool {
  if (cursor + sizeof(storage_type) > target.get_size()) [[unlikely]] {
    return False;
  }

  value = Data::ensure_endian<Data::ByteOrder::Native, endian>(value);
  auto data = target.get_data();
  Data::copy(data + cursor, value);
  cursor += sizeof(storage_type);
  return True;
}

// memcpy is actually too smart for it's own good and will try to interpret
// bytes for floats and reals rather than just loading it as is.
template <Data::ByteOrder endian, typename storage_type, typename real_type>
static constexpr auto
    write_real(Access::Bytes target, Count& cursor, real_type value)
        -> Bool {  // Make unit tests fail at least on mismatch.
  if constexpr (sizeof(storage_type) != sizeof(real_type)) {
    return False;
  }

  if (cursor + sizeof(storage_type) > target.get_size()) [[unlikely]] {
    return False;
  }

  storage_type actual_bytes = *Data::cast<storage_type>(&value);
  actual_bytes =
      Data::ensure_endian<Data::ByteOrder::Native, endian>(actual_bytes);
  auto data = target.get_data();
  Data::copy(data + cursor, actual_bytes);
  cursor += sizeof(storage_type);
  return True;
}

template <Data::ByteOrder endian, typename blob_type>
static constexpr auto
    write_vector(Access::Bytes target, Count& cursor, blob_type blob) -> Bool {
  using element_type = typename blob_type::data_type;

  Count total = sizeof(element_type) * blob.get_size();
  if (cursor + total > target.get_size()) [[unlikely]] {
    return False;
  }

  // Optimize for View::Bytes and when writing in the native endianness.
  auto data = target.get_data();
  if constexpr (
      sizeof(element_type) == 1 || Data::ByteOrder::Native == endian) {
    Data::copy(data + cursor, blob.get_data(), blob.get_size());
    cursor += total;
  } else {
    for (Count index = 0; index < blob.get_size(); index++) {
      element_type element =
          Data::ensure_endian<Data::ByteOrder::Native, endian>(
              blob.get_data()[index]);
      Data::copy(data + cursor, &element);
      cursor += sizeof(element_type);
    }
  }
  return True;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::set_pointer(Count location) -> void {
  cursor = location < source.get_size() ? location : source.get_size();
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const Bits_8 value) -> Binary& {
  valid_state &= write_value<stream_endian>(source, cursor, value);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const Bits_16 value) -> Binary& {
  valid_state &= write_value<stream_endian>(source, cursor, value);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const Bits_32 value) -> Binary& {
  valid_state &= write_value<stream_endian>(source, cursor, value);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const Bits_64 value) -> Binary& {
  valid_state &= write_value<stream_endian>(source, cursor, value);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const Signed_8 value)
    -> Binary& {
  valid_state &= write_value<stream_endian>(source, cursor, value);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const Signed_16 value)
    -> Binary& {
  valid_state &= write_value<stream_endian>(source, cursor, value);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const Signed_32 value)
    -> Binary& {
  valid_state &= write_value<stream_endian>(source, cursor, value);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const Signed_64 value)
    -> Binary& {
  valid_state &= write_value<stream_endian>(source, cursor, value);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const Real_32 value) -> Binary& {
  valid_state &= write_real<stream_endian, Bits_32>(source, cursor, value);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const Real_64 value) -> Binary& {
  valid_state &= write_real<stream_endian, Bits_64>(source, cursor, value);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const View::Bytes raw)
    -> Binary& {
  if (cursor + raw.get_size() > source.get_size()) [[unlikely]] {
    valid_state = False;
    return *this;
  }
  auto data = source.get_data();
  Data::copy(data + cursor, raw.get_data(), raw.get_size());
  cursor += raw.get_size();
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const View::Vector<Bits_8> blob)
    -> Binary& {
  valid_state &= write_vector<stream_endian>(source, cursor, blob);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const View::Vector<Bits_16> blob)
    -> Binary& {
  valid_state &= write_vector<stream_endian>(source, cursor, blob);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const View::Vector<Bits_32> blob)
    -> Binary& {
  valid_state &= write_vector<stream_endian>(source, cursor, blob);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(const View::Vector<Bits_64> blob)
    -> Binary& {
  valid_state &= write_vector<stream_endian>(source, cursor, blob);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(
    const View::Vector<Signed_8> blob) -> Binary& {
  valid_state &= write_vector<stream_endian>(source, cursor, blob);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(
    const View::Vector<Signed_16> blob) -> Binary& {
  valid_state &= write_vector<stream_endian>(source, cursor, blob);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(
    const View::Vector<Signed_32> blob) -> Binary& {
  valid_state &= write_vector<stream_endian>(source, cursor, blob);
  return *this;
}

template <Data::ByteOrder stream_endian>
auto Writer::Binary<stream_endian>::operator<<(
    const View::Vector<Signed_64> blob) -> Binary& {
  valid_state &= write_vector<stream_endian>(source, cursor, blob);
  return *this;
}

template class Writer::Binary<Data::ByteOrder::Little>;
template class Writer::Binary<Data::ByteOrder::Big>;
