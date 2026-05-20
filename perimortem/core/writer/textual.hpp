// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/access/bytes.hpp"

namespace Perimortem::Core::Writer {

class Textual {
 public:
  Textual(Core::Access::Bytes source) : data(source) {};
  Textual(const Textual& rhs) : data(rhs.data) {};

  // Sets the location of the read/write pointer.
  // If the index is out of range then the pointer is put to the end of the
  // buffer.
  auto set_pointer(Count location) -> void;

  auto operator<<(const Byte character) -> Textual&;
  auto operator<<(const Bool flag) -> Textual&;
  auto operator<<(const Half half) -> Textual&;
  auto operator<<(const UHalf unsigned_half) -> Textual&;
  auto operator<<(const Int integer) -> Textual&;
  auto operator<<(const UInt unsigned_integer) -> Textual&;
  auto operator<<(const Long full) -> Textual&;
  auto operator<<(const ULong unsigned_full) -> Textual&;
  auto operator<<(const Real_32 real_32) -> Textual&;
  auto operator<<(const Real_64 real_64) -> Textual&;
  auto operator<<(const Core::View::Bytes raw) -> Textual&;

  constexpr auto get_size() const -> Count { return data.get_size(); }
  constexpr auto get_location() const -> Count { return ptr_location; }
  constexpr auto is_valid() const -> Bool { return valid_state; }

 private:
  auto write_real(Real_64 real, Real_64 precision) -> void;
  Core::Access::Bytes data;
  Count ptr_location = 0;
  Bool valid_state = true;
};

}  // namespace Perimortem::Core::Writer
