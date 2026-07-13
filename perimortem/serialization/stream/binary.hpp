// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/data.hpp"

namespace Perimortem::Serialization::Stream {

// Appends packed values to Dynamic::Bytes or Managed::Bytes. Existing bytes
// are never revisited; use Core::Writer::Binary when an output needs patching.
template <
    Perimortem::Core::Data::ByteOrder stream_endian,
    typename storage_type>
class Binary {
 public:
  explicit Binary(storage_type& storage);

  auto operator<<(Bits_8 value) -> Binary&;
  auto operator<<(Bits_16 value) -> Binary&;
  auto operator<<(Bits_32 value) -> Binary&;
  auto operator<<(Bits_64 value) -> Binary&;
  auto operator<<(Signed_8 value) -> Binary&;
  auto operator<<(Signed_16 value) -> Binary&;
  auto operator<<(Signed_32 value) -> Binary&;
  auto operator<<(Signed_64 value) -> Binary&;
  auto operator<<(Real_32 value) -> Binary&;
  auto operator<<(Real_64 value) -> Binary&;
  auto operator<<(Perimortem::Core::View::Bytes blob) -> Binary&;
  auto operator<<(Perimortem::Core::View::Vector<Bits_8> blob) -> Binary&;
  auto operator<<(Perimortem::Core::View::Vector<Bits_16> blob) -> Binary&;
  auto operator<<(Perimortem::Core::View::Vector<Bits_32> blob) -> Binary&;
  auto operator<<(Perimortem::Core::View::Vector<Bits_64> blob) -> Binary&;
  auto operator<<(Perimortem::Core::View::Vector<Signed_8> blob) -> Binary&;
  auto operator<<(Perimortem::Core::View::Vector<Signed_16> blob) -> Binary&;
  auto operator<<(Perimortem::Core::View::Vector<Signed_32> blob) -> Binary&;
  auto operator<<(Perimortem::Core::View::Vector<Signed_64> blob) -> Binary&;

 private:
  storage_type& storage;
};

}  // namespace Perimortem::Serialization::Stream
