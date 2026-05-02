// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/serialization/textual/stream.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Serialization;

// charconv is one of few super optimized std implementations to the point that
// it's better to pull it in for this one instance.
//
// The build time hit for the few files we use it in is worth it, but it should
// never be included in a Perimortem header file.
#include <charconv>

template <typename text_buffer>
constexpr auto write_text(Access::Bytes& data,
                          Count& ptr_location,
                          const text_buffer& text) -> Bool {
  if (ptr_location + text.get_size() >= data.get_size()) {
    return false;
  }

  Data::copy(data.get_data() + ptr_location, text.get_data(), text.get_size());
  ptr_location += text.get_size();

  return true;
}

template <typename storage_type>
constexpr auto to_chars(Access::Bytes& data,
                        Count& ptr_location,
                        const storage_type& value) -> Bool {
  auto result = std::to_chars(
      Data::cast<char>(data.get_data() + ptr_location),
      Data::cast<char>(data.get_data() + data.get_size()), value);

  if (result) {
    ptr_location = result.ptr - Data::cast<char>(data.get_data());
    return true;
  } else {
    return false;
  }
}

auto Textual::Stream::operator<<(const Byte character) -> Textual::Stream& {
  valid_state &= ptr_location < data.get_size();
  if (valid_state) {
    data.get_data()[ptr_location++] = character;
  }

  return *this;
}

auto Textual::Stream::operator<<(const Bool boolean) -> Textual::Stream& {
  if (boolean) {
    valid_state &= write_text(data, ptr_location, "true"_view);
  } else {
    valid_state &= write_text(data, ptr_location, "false"_view);
  }

  return *this;
}

auto Textual::Stream::operator<<(const Half half) -> Textual::Stream& {
  valid_state &= to_chars(data, ptr_location, half);
  return *this;
}

auto Textual::Stream::operator<<(const UHalf unsigned_half)
    -> Textual::Stream& {
  valid_state &= to_chars(data, ptr_location, unsigned_half);
  return *this;
}

auto Textual::Stream::operator<<(const Int integer) -> Textual::Stream& {
  valid_state &= to_chars(data, ptr_location, integer);
  return *this;
}

auto Textual::Stream::operator<<(const UInt unsigned_integer)
    -> Textual::Stream& {
  valid_state &= to_chars(data, ptr_location, unsigned_integer);
  return *this;
}

auto Textual::Stream::operator<<(const Long full) -> Textual::Stream& {
  valid_state &= to_chars(data, ptr_location, full);
  return *this;
}

auto Textual::Stream::operator<<(const ULong unsigned_full)
    -> Textual::Stream& {
  valid_state &= to_chars(data, ptr_location, unsigned_full);
  return *this;
}

auto Textual::Stream::operator<<(const Real_32 real_32) -> Textual::Stream& {
  valid_state &= to_chars(data, ptr_location, real_32);
  return *this;
}

auto Textual::Stream::operator<<(const Real_64 Real_64) -> Textual::Stream& {
  valid_state &= to_chars(data, ptr_location, Real_64);
  return *this;
}

auto Textual::Stream::operator<<(const Core::View::Bytes raw)
    -> Textual::Stream& {
  valid_state &= write_text(data, ptr_location, raw);
  return *this;
}
