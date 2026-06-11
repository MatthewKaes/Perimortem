// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/reader/binary.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/writer/textual.hpp"

using namespace Perimortem::Core;

auto check_buffer_overruns(Count ptr, Count source_size, Count read_size)
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

template <Data::ByteOrder endian, typename storage_type>
static constexpr auto read_value(View::Bytes source, Bool& valid, Count& ptr)
    -> storage_type {
  Count base = Count(source.get_data());
  ptr = Data::align<sizeof(storage_type)>(base + ptr) - base;

  if (!check_buffer_overruns(ptr, source.get_size(), sizeof(storage_type))) {
    valid = false;
    return storage_type();
  }

  storage_type actual_bytes;
  memcpy(&actual_bytes, source.get_data() + ptr, sizeof(storage_type));
  ptr += sizeof(storage_type);
  return Data::ensure_endian<endian, Data::ByteOrder::Native>(actual_bytes);
}

// memcpy is actually too smart for it's own good and will try to interpret
// bytes for floats and reals rather than just loading it as is.
template <Data::ByteOrder endian, typename storage_type, typename real_type>
static constexpr auto read_real(View::Bytes source, Bool& valid, Count& ptr)
    -> real_type {
  // Make unit tests fail at least on mismatch.
  if constexpr (sizeof(storage_type) != sizeof(real_type)) {
    valid = False;
    return real_type();
  }

  Count base = Count(source.get_data());
  ptr = Data::align<sizeof(storage_type)>(base + ptr) - base;

  if (!check_buffer_overruns(ptr, source.get_size(), sizeof(storage_type))) {
    valid = False;
    return real_type();
  }

  storage_type actual_bytes;
  memcpy(&actual_bytes, source.get_data() + ptr, sizeof(storage_type));
  actual_bytes =
      Data::ensure_endian<endian, Data::ByteOrder::Native>(actual_bytes);

  // Read the bytes in correct memory order into the floating point unit.
  ptr += sizeof(storage_type);
  return *Data::cast<real_type>(&actual_bytes);
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::set_pointer(Count location) -> void {
  ptr_location = location < data.get_size() ? location : data.get_size();
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_bits_8() -> Bits_8 {
  return read_value<stream_endian, Bits_8>(data, valid_state, ptr_location);
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_bits_16() -> Bits_16 {
  return read_value<stream_endian, Bits_16>(data, valid_state, ptr_location);
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_bits_32() -> Bits_32 {
  return read_value<stream_endian, Bits_32>(data, valid_state, ptr_location);
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_bits_64() -> Bits_64 {
  return read_value<stream_endian, Bits_64>(data, valid_state, ptr_location);
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_signed_bits_8() -> Signed_8 {
  return read_value<stream_endian, Signed_8>(data, valid_state, ptr_location);
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_signed_bits_16() -> Signed_16 {
  return read_value<stream_endian, Signed_16>(data, valid_state, ptr_location);
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_signed_bits_32() -> Signed_32 {
  return read_value<stream_endian, Signed_32>(data, valid_state, ptr_location);
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_signed_bits_64() -> Signed_64 {
  return read_value<stream_endian, Signed_64>(data, valid_state, ptr_location);
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_real_32() -> Real_32 {
  return read_real<stream_endian, Bits_32, Real_32>(
      data, valid_state, ptr_location);
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_real_64() -> Real_64 {
  return read_real<stream_endian, Bits_64, Real_64>(
      data, valid_state, ptr_location);
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
