// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/utility/func/data.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Memory::Allocator;
using namespace Perimortem::Utility::Func;

constexpr auto access(Bibliotheca::Preface* block) -> Byte* {
  return Bibliotheca::preface_to_corpus(block);
}

Dynamic::Bytes::Bytes() : size(0), rented_block(nullptr) {}
Dynamic::Bytes::Bytes(Count reserved_capacity) {
  rented_block = Bibliotheca::check_out(reserved_capacity);
  size = 0;
}

Dynamic::Bytes::Bytes(const Core::View::Amorphous view) {
  size = view.get_size();
  if (view.get_size() == 0) {
    return;
  }

  rented_block = Bibliotheca::check_out(view.get_size());
  size = view.get_size();
  memcpy(access(rented_block), view.get_data(), view.get_size());
}

Dynamic::Bytes::Bytes(const Bytes& rhs) {
  size = rhs.size;
  if (rhs.size == 0) {
    return;
  }

  // Bytes don't require any special handling so just memcpy.
  rented_block = Bibliotheca::check_out(rhs.size);
  memcpy(access(rented_block), access(rhs.rented_block), size);
};

Dynamic::Bytes::Bytes(Bytes&& rhs) {
  size = rhs.size;
  rented_block = rhs.rented_block;

  rhs.rented_block = nullptr;
  rhs.size = 0;
};

Dynamic::Bytes::~Bytes() {
  reset();
}

auto Dynamic::Bytes::append(Byte b) -> void {
  ensure_capacity(size + 1);
  access(rented_block)[size++] = b;
}

auto Dynamic::Bytes::concat(Core::View::Amorphous view) -> void {
  ensure_capacity(size + view.get_size());

  Data::copy(access(rented_block) + size, view.get_data(), view.get_size());
  size += view.get_size();
}

auto Dynamic::Bytes::proxy(Core::View::Amorphous view) -> void {
  ensure_capacity(view.get_size());

  Data::copy(access(rented_block), view.get_data(), view.get_size());
  size = view.get_size();
}

auto Dynamic::Bytes::convert(Byte source, Byte target) -> void {
  for (int i = 0; i < size; i++) {
    if (access(rented_block)[i] == source) {
      access(rented_block)[i] = target;
    }
  }
}

auto Dynamic::Bytes::slice(Count start, Count size) const
    -> Core::View::Amorphous {
  if (start >= get_size())
    return Core::View::Amorphous();

  return Core::View::Amorphous(Core::View::Amorphous(
      Allocator::Bibliotheca::preface_to_corpus(rented_block) + start,
      Math::min(size, get_size() - start)));
}

auto Dynamic::Bytes::resize(Count new_size) -> void {
  ensure_capacity(new_size);
  size = new_size;
}

auto Dynamic::Bytes::operator[](Count index) const -> Byte {
  if (index > size)
    return 0;

  return access(rented_block)[index];
}

auto Dynamic::Bytes::at(Count index) const -> Byte {
  if (index > size)
    return 0;

  return access(rented_block)[index];
}

auto Dynamic::Bytes::clear() -> void {
  size = 0;
}

auto Dynamic::Bytes::reset() -> void {
  size = 0;

  // In the event the data was moved.
  if (rented_block) {
    Bibliotheca::remit(rented_block);
  }
}

auto Dynamic::Bytes::ensure_capacity(Count required_size) -> void {
  // Check if we can already fit required buffer.
  if (required_size <= get_capacity()) {
    return;
  }

  // Attempt to grow by a factor of 2.
  // If that doesn't work than grow to exact size.
  const auto new_capacity = Math::max(get_capacity() * 2, required_size);

  // Fetch and transfer to new block.
  auto new_block = Bibliotheca::check_out(new_capacity);

  if (rented_block) {
    memcpy(access(new_block), access(rented_block), size);
    Bibliotheca::remit(rented_block);
  }

  // Update block and get the new capacity.
  rented_block = new_block;
}
