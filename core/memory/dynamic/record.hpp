// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "core/memory/bibliotheca.hpp"
#include "core/memory/managed/buffer.hpp"

namespace Perimortem::Memory::Dynamic {

// Record are similar to std::shared_ptr with a thread safe control block but
// with a few key differences:
//
// Records only account for some raw memory buffer that is untyped. There are no
// destructors or callbacks, on multiple consumers.
//
// Records size is 32 (preface size) + sizeof(T) rounded up to the next power
// of 2 by using Bibliotheca.
//
// Records are automatically pooled _by size_ with most recently touched blocks
// being the first to be reused.
//
//
// ** It's important to not put Records or Dynamic objects into Managed objects.
class Record {
 public:
  Record() : reserved_block(nullptr) {};
  Record(Count capacity) : size(capacity) { reserved_block = Bibliotheca::check_out(capacity); };

  Record(const Record& rhs) {
    Bibliotheca::reserve(rhs.reserved_block);
    reserved_block = rhs.reserved_block;
  }

  auto operator=(const Record& rhs) {
    if (!reserved_block) {
      Bibliotheca::reserve(rhs.reserved_block);
    } else {
      Bibliotheca::exchange(reserved_block, rhs.reserved_block);
    }
    reserved_block = rhs.reserved_block;
    size = rhs.size;
  }

  ~Record() { clear(); }

  auto clear() -> void {
    if (!reserved_block) {
      return;
    }

    Bibliotheca::remit(reserved_block);
    reserved_block = nullptr;
  }

  auto get_buffer() const -> Managed::Buffer {
    if (!reserved_block)
      return Managed::Buffer();

    return Managed::Buffer(Bibliotheca::preface_to_corpus(reserved_block), size);
  }

  auto get_size() const -> Count {
    return size;
  }

  auto set_size(Count size) -> void {
    this->size = std::min(size, reserved_block->capacity);
  }

  auto get_capacity() const -> Count {
    return reserved_block->capacity - reserved_block->usage;
  }

 private:
  Bibliotheca::Preface* reserved_block = nullptr;
  Count size;
};

}  // namespace Perimortem::Memory::Dynamic
