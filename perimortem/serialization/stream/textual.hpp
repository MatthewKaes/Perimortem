// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

namespace Perimortem::Serialization::Stream {

// Appends Core::Writer::Textual output to Dynamic::Bytes or Managed::Bytes.
template <typename storage_type>
class Textual {
 public:
  constexpr Textual(storage_type& storage) : storage(storage) {}

  auto operator<<(Bool flag) -> Textual&;
  auto operator<<(Unsigned_8 byte) -> Textual&;
  auto operator<<(Unsigned_16 value) -> Textual&;
  auto operator<<(Unsigned_32 value) -> Textual&;
  auto operator<<(Unsigned_64 value) -> Textual&;
  auto operator<<(Signed_8 value) -> Textual&;
  auto operator<<(Signed_16 value) -> Textual&;
  auto operator<<(Signed_32 value) -> Textual&;
  auto operator<<(Signed_64 value) -> Textual&;
  auto operator<<(Real_32 value) -> Textual&;
  auto operator<<(Real_64 value) -> Textual&;
  auto operator<<(Perimortem::Core::View::Bytes raw) -> Textual&;
  auto operator<<(const char* raw) -> Textual& = delete;

 private:
  storage_type& storage;
};

}  // namespace Perimortem::Serialization::Stream
