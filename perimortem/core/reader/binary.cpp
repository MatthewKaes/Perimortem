// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/reader/binary.hpp"

#include "perimortem/core/data.hpp"

using namespace Perimortem::Core;

// read_value is a local template helper so the member implementations can
// stay in this .cpp file without leaking into the header.
template <Data::ByteOrder endian, typename storage_type>
static constexpr auto
    read_value(View::Bytes source, Count& ptr, storage_type& out) -> Bool {
  Count base = Count(source.get_data());
  ptr = Data::align<sizeof(storage_type)>(base + ptr) - base;

  if (ptr + sizeof(storage_type) > source.get_size()) [[unlikely]] {
    return False;
  }

  storage_type raw = {};
  memcpy(&raw, source.get_data() + ptr, sizeof(storage_type));
  out = Data::ensure_endian<endian, Data::ByteOrder::Native>(raw);
  ptr += sizeof(storage_type);
  return True;
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::set_pointer(Count location) -> void {
  ptr_location = location < data.get_size() ? location : data.get_size();
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_bits_8() -> Bits_8 {
  Bits_8 out = 0;
  valid_state &= read_value<stream_endian>(data, ptr_location, out);
  return out;
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_bits_16() -> Bits_16 {
  Bits_16 out = 0;
  valid_state &= read_value<stream_endian>(data, ptr_location, out);
  return out;
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_bits_32() -> Bits_32 {
  Bits_32 out = 0;
  valid_state &= read_value<stream_endian>(data, ptr_location, out);
  return out;
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_bits_64() -> Bits_64 {
  Bits_64 out = 0;
  valid_state &= read_value<stream_endian>(data, ptr_location, out);
  return out;
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_signed_bits_8() -> SignedBits_8 {
  SignedBits_8 out = 0;
  valid_state &= read_value<stream_endian>(data, ptr_location, out);
  return out;
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_signed_bits_16() -> SignedBits_16 {
  SignedBits_16 out = 0;
  valid_state &= read_value<stream_endian>(data, ptr_location, out);
  return out;
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_signed_bits_32() -> SignedBits_32 {
  SignedBits_32 out = 0;
  valid_state &= read_value<stream_endian>(data, ptr_location, out);
  return out;
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_signed_bits_64() -> SignedBits_64 {
  SignedBits_64 out = 0;
  valid_state &= read_value<stream_endian>(data, ptr_location, out);
  return out;
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_real_32() -> Real_32 {
  Real_32 out = Real_32(0);
  valid_state &= read_value<stream_endian>(data, ptr_location, out);
  return out;
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_real_64() -> Real_64 {
  Real_64 out = Real_64(0);
  valid_state &= read_value<stream_endian>(data, ptr_location, out);
  return out;
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_bytes(Count count) -> View::Bytes {
  if (ptr_location + count > data.get_size()) [[unlikely]] {
    valid_state = False;
    return data.slice(data.get_size(), 0);
  }
  View::Bytes result = data.slice(ptr_location, count);
  ptr_location += count;
  return result;
}

// Instantiate the two binary readers.
// We don't need to instantiate Native as it should alias to the correct
// implementation for the system.
template class Reader::Binary<Data::ByteOrder::Little>;
template class Reader::Binary<Data::ByteOrder::Big>;
