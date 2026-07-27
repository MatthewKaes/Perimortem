// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/reader/binary.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;

auto check_buffer_overruns(Count cursor, Count source_size, Count read_size)
    -> Bool {
  if (cursor > source_size || read_size > source_size - cursor) [[unlikely]] {
    Diagnostics::Log::Message<128> error_message(
        Diagnostics::Log::Level::Error);
    error_message << "Binary read over ran buffer at read location "_view
                  << cursor << ". source_size="_view << source_size
                  << ", read_size="_view << read_size;
    return False;
  }

  return True;
}

template <Data::ByteOrder endian, typename storage_type>
static constexpr auto read_value(View::Bytes source, Count& cursor)
    -> storage_type {
  if (cursor == Count(-1)) {
    return storage_type();
  }

  if (!check_buffer_overruns(cursor, source.get_size(), sizeof(storage_type))) {
    cursor = Count(-1);
    return storage_type();
  }

  storage_type actual_bytes;
  memcpy(&actual_bytes, source.get_data() + cursor, sizeof(storage_type));
  cursor += sizeof(storage_type);
  return Data::ensure_endian<endian, Data::ByteOrder::Native>(actual_bytes);
}

// memcpy is actually too smart for it's own good and will try to interpret
// bytes for floats and reals rather than just loading it as is.
template <Data::ByteOrder endian, typename storage_type, typename real_type>
static constexpr auto read_real(View::Bytes source, Count& cursor)
    -> real_type {
  if (cursor == Count(-1)) {
    return real_type();
  }

  // Make unit tests fail at least on mismatch.
  if constexpr (sizeof(storage_type) != sizeof(real_type)) {
    cursor = Count(-1);
    return real_type();
  }

  if (!check_buffer_overruns(cursor, source.get_size(), sizeof(storage_type))) {
    cursor = Count(-1);
    return real_type();
  }

  storage_type actual_bytes;
  memcpy(&actual_bytes, source.get_data() + cursor, sizeof(storage_type));
  actual_bytes =
      Data::ensure_endian<endian, Data::ByteOrder::Native>(actual_bytes);

  // Read the bytes in correct memory order into the floating point unit.
  cursor += sizeof(storage_type);
  return *Data::cast<real_type>(&actual_bytes);
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_unsigned_8() -> Unsigned_8 {
  return read_value<stream_endian, Unsigned_8>(source, cursor);
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_unsigned_16() -> Unsigned_16 {
  return read_value<stream_endian, Unsigned_16>(source, cursor);
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_unsigned_32() -> Unsigned_32 {
  return read_value<stream_endian, Unsigned_32>(source, cursor);
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_unsigned_64() -> Unsigned_64 {
  return read_value<stream_endian, Unsigned_64>(source, cursor);
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_signed_8() -> Signed_8 {
  return read_value<stream_endian, Signed_8>(source, cursor);
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_signed_16() -> Signed_16 {
  return read_value<stream_endian, Signed_16>(source, cursor);
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_signed_32() -> Signed_32 {
  return read_value<stream_endian, Signed_32>(source, cursor);
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_signed_64() -> Signed_64 {
  return read_value<stream_endian, Signed_64>(source, cursor);
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_real_32() -> Real_32 {
  return read_real<stream_endian, Unsigned_32, Real_32>(source, cursor);
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_real_64() -> Real_64 {
  return read_real<stream_endian, Unsigned_64, Real_64>(source, cursor);
}

template <Data::ByteOrder stream_endian>
auto Reader::Binary<stream_endian>::read_bytes(Count count) -> View::Bytes {
  if (cursor == Count(-1)) {
    return View::Bytes();
  }

  if (!check_buffer_overruns(cursor, source.get_size(), count)) {
    cursor = Count(-1);
    return View::Bytes();
  }

  View::Bytes result = source.slice(cursor, count);
  cursor += count;
  return result;
}

// Instantiate the two binary readers.
// We don't need to instantiate Native as it should alias to the correct
// implementation for the system.
template class Reader::Binary<Data::ByteOrder::Little>;
template class Reader::Binary<Data::ByteOrder::Big>;
