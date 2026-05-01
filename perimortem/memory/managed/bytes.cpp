
// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/core/math/math.hpp"

#include <x86intrin.h>

using namespace Perimortem::Memory;

Managed::Bytes::Bytes(const Bytes& rhs, Count reserved_capacity)
    : arena(rhs.arena) {
  reset(Core::Math::max(rhs.get_size(), reserved_capacity));
  proxy(rhs);
};

Managed::Bytes::Bytes(Bytes&& rhs) : arena(rhs.arena) {
  // Take ownership and invalidate the old
  size = rhs.size;
  capacity = rhs.capacity;
  rented_block = rhs.rented_block;
};

Managed::Bytes::Bytes(Allocator::Arena& arena) : arena(arena) {
  reset();
}

auto Managed::Bytes::reset(Count reserved_capacity) -> void {
  size = 0;
  capacity = reserved_capacity;
  rented_block = reinterpret_cast<Byte*>(arena.allocate(reserved_capacity));
}

auto Managed::Bytes::resize(Count new_size) -> void {
  ensure_capacity(new_size);
  size = new_size;
}

auto Managed::Bytes::append(Byte b) -> void {
  ensure_capacity(size + 1);
  rented_block[size++] = b;
}

auto Managed::Bytes::append(Core::View::Amorphous view) -> void {
  ensure_capacity(size + view.get_size());

  memcpy(rented_block + size, view.get_data(), view.get_size());
  size += view.get_size();
}

auto Managed::Bytes::proxy(Core::View::Amorphous view) -> void {
  if (view.get_size() > capacity)
    grow(view.get_size() - capacity);

  memcpy(rented_block + size, view.get_data(), view.get_size());
  size = view.get_size();
}

auto Managed::Bytes::convert(Byte source, Byte target) -> void {
  for (int i = 0; i < size; i++) {
    if (rented_block[i] == source) {
      rented_block[i] = target;
    }
  }
}

auto Managed::Bytes::ensure_capacity(Count required_bytes) -> void {
  // Check if we can already fit required buffer.
  if (required_bytes <= capacity) {
    return;
  }

  // Attempt to grow by a factor of 2.
  // If that doesn't work than grow to exact size.
  const auto new_capacity = Core::Math::max(capacity * 2, required_bytes);

  // Fetch and transfer to new block.
  auto new_block = reinterpret_cast<Byte*>(arena.allocate(new_capacity));

  // Update block and get the new capacity.
  memcpy(new_block, rented_block, size);
  rented_block = new_block;
}

auto Managed::Bytes::grow(Count requested) -> void {
  const auto required = capacity + requested;
  // Attempt to grow by a factor of 2.
  // If that doesn't work than grow to exact size.
  capacity *= growth_factor;
  if (capacity < required) {
    capacity = required;
  }

  auto new_block = reinterpret_cast<Byte*>(arena.allocate(capacity));

  memcpy(new_block, rented_block, size);
  rented_block = new_block;
}
