// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

namespace Perimortem::Abi::Memory::Dynamic {

// Bytes exposes the native symbols and physical carrier used by generated
// code. Memory::Dynamic::Bytes remains the owning Perimortem value. The carrier
// exposes read only storage so code crossing the ABI cannot bypass copy on
// write detachment. A uniquely owned value may still mutate through its owning
// Memory operation. Each call signature determines whether the carrier is
// borrowed or transferred. Retain and release observe the complete carrier
// selected by authored Type Attributes. This interface adds no second lifetime
// or storage implementation.
class Bytes {
 public:
  static constexpr auto
      create(const Unsigned_8* data, Count size, Count capacity) -> Bytes {
    Bytes result = {};
    result.data = data;
    result.size = size;
    result.capacity = capacity;
    return result;
  }

  static constexpr Perimortem::Core::View::Bytes retain_symbol =
      "perimortem_dynamic_bytes_retain"_view;
  static constexpr Perimortem::Core::View::Bytes release_symbol =
      "perimortem_dynamic_bytes_release"_view;
  static constexpr Perimortem::Core::View::Bytes concat_symbol =
      "perimortem_dynamic_bytes_concat"_view;

  constexpr auto get_data() const -> const Unsigned_8* { return data; }
  constexpr auto get_size() const -> Count { return size; }
  constexpr auto get_capacity() const -> Count { return capacity; }

 private:
  const Unsigned_8* data;
  Count size;
  Count capacity;
};

static_assert(sizeof(Bytes) == sizeof(Unsigned_8*) + sizeof(Count) * 2);
static_assert(alignof(Bytes) == alignof(Count));
static_assert(__is_trivial(Bytes));
static_assert(__is_standard_layout(Bytes));

}  // namespace Perimortem::Abi::Memory::Dynamic

extern "C" auto perimortem_dynamic_bytes_retain(
    const Perimortem::Abi::Memory::Dynamic::Bytes* value) -> void;
extern "C" auto perimortem_dynamic_bytes_release(
    const Perimortem::Abi::Memory::Dynamic::Bytes* value) -> void;

extern "C" auto perimortem_dynamic_bytes_concat(
    Perimortem::Memory::Dynamic::Bytes* output,
    Perimortem::Core::View::Bytes left,
    Perimortem::Core::View::Bytes right) -> void;
