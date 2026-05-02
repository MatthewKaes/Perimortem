// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/core/data.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Memory::Allocator;

constexpr auto access(Bibliotheca::Preface* block) -> Byte* {
  return Bibliotheca::preface_to_corpus(block);
}

Dynamic::Bytes::Bytes() : size(0), source_block(nullptr) {}
Dynamic::Bytes::Bytes(Count reserved_capacity) {
  source_block = access(Bibliotheca::check_out(reserved_capacity));
  size = 0;
}

Dynamic::Bytes::Bytes(const Core::View::Bytes view) {
  size = view.get_size();
  if (view.get_size() == 0) {
    return;
  }

  source_block = access(Bibliotheca::check_out(view.get_size()));
  size = view.get_size();
  memcpy(source_block, view.get_data(), view.get_size());
}

Dynamic::Bytes::Bytes(const Bytes& rhs) {
  size = rhs.size;
  if (rhs.size == 0) {
    return;
  }

  // Bytes don't require any special handling so just memcpy.
  source_block = access(Bibliotheca::check_out(rhs.size));
  memcpy(source_block, rhs.source_block, size);
};

Dynamic::Bytes::Bytes(Bytes&& rhs) {
  size = rhs.size;
  source_block = rhs.source_block;

  rhs.source_block = nullptr;
  rhs.size = 0;
};

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
  ensure_capacity(view.get_size());

  Data::copy(source_block, view.get_data(), view.get_size());
  size = view.get_size();
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
    Bibliotheca::remit(Bibliotheca::corpus_to_preface(source_block));
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
  auto new_block = access(Bibliotheca::check_out(new_capacity));

  if (source_block) {
    memcpy(new_block, source_block, size);
    Bibliotheca::remit(Bibliotheca::corpus_to_preface(source_block));
  }

  // Update block and get the new capacity.
  source_block = new_block;
}
