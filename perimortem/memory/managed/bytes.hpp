// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/view/bytes.hpp"

namespace Perimortem::Memory::Managed {

// A simple linear string which supports historical views on old data
// as long as the associated Arena is still alive.
class Bytes {
 public:
  static constexpr Count start_capacity = 32;
  static constexpr Count growth_factor = 2;

  Bytes(const Bytes& rhs, Count reserved_capacity = start_capacity);
  Bytes(Bytes&& rhs);
  Bytes(Allocator::Arena& arena);

  constexpr operator View::Bytes() const {
    return View::Bytes(rented_block, size);
  }

  auto reset(Count reserved_capacity = start_capacity) -> void;

  auto ensure_room(Count required_bytes) -> void;
  auto resize(Count new_size) -> void;
  auto ensure_capacity(Count required_bytes) -> void;

  // Copies a View::Bytes which may be in a different allocator (dynamic or
  // another arena) into the Arena used by this object.
  auto proxy(View::Bytes view) -> void;

  auto append(Byte b) -> void;
  auto append(View::Bytes view) -> void;
  auto convert(Byte source, Byte target) -> void;

  constexpr auto operator[](Count index) const -> Byte {
    if (index > size)
      return 0;

    return rented_block[index];
  }

  constexpr auto at(Count index) const -> Byte {
    if (index > size)
      return 0;

    return rented_block[index];
  }

  constexpr auto get_size() const -> Count { return size; }
  constexpr auto get_view() const -> const View::Bytes {
    return View::Bytes(rented_block, size);
  }
  constexpr auto get_data() const -> Byte* {
    return rented_block;
  }
  constexpr auto get_arena() const -> Allocator::Arena& { return arena; }

 private:
  auto grow(Count requested) -> void;

  Allocator::Arena& arena;
  Byte* rented_block;
  Count size;
  Count capacity;
};

}  // namespace Perimortem::Memory::Managed
