// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/data.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;

Dynamic::Bytes::Bytes() {}
Dynamic::Bytes::Bytes(Count reserved_capacity) {
  auto alloc = Bibliotheca::check_out(reserved_capacity);
  source_block = alloc.ptr;
  capacity = alloc.capacity;
}

Dynamic::Bytes::Bytes(const Core::View::Bytes view) : size(view.get_size()) {
  if (view.get_size() == 0) {
    capacity = 0;
    source_block = nullptr;
    return;
  }

  auto alloc = Bibliotheca::check_out(view.get_size());
  source_block = alloc.ptr;
  capacity = alloc.capacity;
  memcpy(source_block, view.get_data(), view.get_size());
}

Dynamic::Bytes::Bytes(const Bytes& rhs) {
  capacity = rhs.capacity;
  size = rhs.size;
  if (rhs.size == 0) {
    return;
  }

  // Bytes don't require any special handling so just memcpy.
  auto alloc = Bibliotheca::check_out(rhs.size);
  source_block = alloc.ptr;
  capacity = alloc.capacity;
  memcpy(source_block, rhs.source_block, size);
}

Dynamic::Bytes::Bytes(Bytes&& rhs) {
  size = rhs.size;
  capacity = rhs.capacity;
  source_block = rhs.source_block;

  rhs.source_block = nullptr;
  rhs.size = 0;
  rhs.capacity = 0;
}

auto Dynamic::Bytes::operator=(Core::View::Bytes view) -> Bytes& {
  proxy(view);
  return *this;
}

auto Dynamic::Bytes::operator=(const Bytes& rhs) -> Bytes& {
  proxy(rhs);
  return *this;
}

auto Dynamic::Bytes::operator=(Bytes&& rhs) -> Bytes& {
  size = rhs.size;
  source_block = rhs.source_block;

  rhs.source_block = nullptr;
  rhs.size = 0;
  return *this;
}

Dynamic::Bytes::~Bytes() {
  reset();
}

auto Dynamic::Bytes::append(Byte b) -> void {
  ensure_capacity(size + 1);
  source_block[size++] = b;
}

auto Dynamic::Bytes::concat(Core::View::Bytes view) -> void {
  ensure_capacity(size + view.get_size());

  Data::copy(source_block + size, view.get_data(), view.get_size());
  size += view.get_size();
}

auto Dynamic::Bytes::proxy(Core::View::Bytes view) -> void {
  forgetful_resize(view.get_size());

  Data::copy(source_block, view.get_data(), view.get_size());
}

auto Dynamic::Bytes::set(Byte target) -> void {
  Data::set(source_block, target, get_size());
}

auto Dynamic::Bytes::convert(Byte source, Byte target) -> void {
  for (int i = 0; i < size; i++) {
    if (source_block[i] == source) {
      source_block[i] = target;
    }
  }
}

auto Dynamic::Bytes::slice(Count start, Count size) const -> Core::View::Bytes {
  if (start >= get_size())
    return Core::View::Bytes();

  return Core::View::Bytes(source_block + start,
                           Math::min(size, get_size() - start));
}

auto Dynamic::Bytes::resize(Count new_size) -> void {
  ensure_capacity(new_size);
  size = new_size;
}

auto Dynamic::Bytes::forgetful_resize(Count required_size) -> void {
  // Always set the size.
  size = required_size;

  // Get the capacity bounds and check if we need a realloc.
  // If the block fits in the current Bibliotheca archive then reuse it.
  // If the block size requires at least one step up or step down then request a
  // new block.
  if (required_size <= capacity && required_size > (capacity >> 1)) {
    return;
  }

  if (source_block) {
    Core::Bibliotheca::remit(source_block);
  }

  auto alloc = Bibliotheca::check_out(required_size);
  source_block = alloc.ptr;
  capacity = alloc.capacity;
}

auto Dynamic::Bytes::operator[](Count index) const -> Byte {
  if (index > size)
    return 0;

  return source_block[index];
}

auto Dynamic::Bytes::at(Count index) const -> Byte {
  if (index > size)
    return 0;

  return source_block[index];
}

auto Dynamic::Bytes::clear() -> void {
  size = 0;
}

auto Dynamic::Bytes::reset() -> void {
  size = 0;

  // In the event the data was moved.
  if (source_block) {
    Bibliotheca::remit(source_block);
  }
}

auto Dynamic::Bytes::ensure_capacity(Count required_size) -> void {
  // Check if we can already fit required buffer.
  if (required_size <= get_capacity()) {
    return;
  }

  // Since the current block doesn't fit in the current archive fetch and
  // transfer to a new block.
  auto alloc = Bibliotheca::check_out(required_size);

  if (source_block) {
    memcpy(alloc.ptr, source_block, size);
    Bibliotheca::remit(source_block);
  }

  // Update block and get the new capacity.
  source_block = alloc.ptr;
  capacity = alloc.capacity;
}
