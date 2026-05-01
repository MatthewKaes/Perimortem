// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/access/amorphous.hpp"
#include "perimortem/core/view/amorphous.hpp"
#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/math/hash.hpp"

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

  constexpr operator Core::View::Amorphous() const { return get_view(); }
  constexpr operator Core::Access::Amorphous() { return get_access(); }

  auto reset(Count reserved_capacity = start_capacity) -> void;

  auto resize(Count new_size) -> void;
  auto ensure_capacity(Count required_bytes) -> void;

  // Copies a Core::View::Amorphous which may be in a different allocator
  // (dynamic or another arena) into the Arena used by this object.
  auto proxy(Core::View::Amorphous view) -> void;

  auto append(Byte b) -> void;
  auto append(Core::View::Amorphous view) -> void;
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
  constexpr auto get_capacity() const -> Count { return capacity; }
  constexpr auto get_view() const -> const Core::View::Amorphous {
    return Core::View::Amorphous(rented_block, size);
  }
  constexpr auto get_access() -> Core::Access::Amorphous {
    return Core::Access::Amorphous(rented_block, size);
  }
  constexpr auto get_arena() const -> Allocator::Arena& { return arena; }
   
  constexpr auto hash() const -> Bits_64 {
    return Math::Hash(get_view()).get_value();
  }

 private:
  auto grow(Count requested) -> void;

  Allocator::Arena& arena;
  Byte* rented_block;
  Count size;
  Count capacity;
};

}  // namespace Perimortem::Memory::Managed
