// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/reader/binary.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/writer/textual.hpp"

using namespace Perimortem::Core;

static auto check_buffer_overruns(Count ptr, Count source_size, Count read_size)
    -> Bool {
  if (ptr + read_size > source_size) [[unlikely]] {
    Static::Bytes<128> error_buffer;
    Writer::Textual error_message(error_buffer);
    error_message << "Binary read over ran buffer at read location "_view << ptr
                  << ". source_size="_view << source_size << ", read_size="_view
                  << read_size;
    Diagnostics::Log::error(error_message);
    return False;
  }

  return True;
}

// read_value is a local template helper so the member implementations can
// stay in this .cpp file without leaking into the header.
template <Data::ByteOrder endian, typename storage_type>
static constexpr auto
    read_value(View::Bytes source, Count& ptr, storage_type& out) -> Bool {
  Count base = Count(source.get_data());
  ptr = Data::align<sizeof(storage_type)>(base + ptr) - base;

  if (!check_buffer_overruns(ptr, source.get_size(), sizeof(storage_type))) {
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
  if (!check_buffer_overruns(ptr_location, data.get_size(), count)) {
    valid_state = False;
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
