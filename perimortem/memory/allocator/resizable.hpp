// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/bibliotheca.hpp"
#include "perimortem/memory/const/standard_types.hpp"

namespace Perimortem::Memory::Allocator {

// A resizable buffer used to back most dynamic allocator.
template <typename storage_type, Count start_capacity, Count growth_factor = 2>
class Resizable {
 protected:
  auto reset(Count reserved_capacity = start_capacity) -> void { size = 0; }
  auto get_capacity() const -> Count { return capacity; }
  Resizable(Count start_capacity = start_capacity) {
    rented_block =
        Bibliotheca::check_out(start_capacity * sizeof(storage_type));
    capacity = rented_block->usable_bytes() / sizeof(storage_type);
    size = 0;
  }

  Resizable(Count start_capacity = start_capacity) {
    rented_block =
        Bibliotheca::check_out(start_capacity * sizeof(storage_type));
    capacity = rented_block->usable_bytes() / sizeof(storage_type);
    size = 0;
  }

  Resizable(Resizable&& rhs) {
    rented_block = rhs.rented_block;
    capacity = rented_block->usable_bytes() / sizeof(storage_type);
    size = 0;
    rhs.rented_block = nullptr;
  }

  ~Resizable() {
    // In the event the data was moved.
    if (rented_block) {
      Bibliotheca::remit(rented_block);
    }
  }

  static constexpr auto min_capacity() -> Count { return start_capacity; }

  auto access() -> storage_type* {
    return std::bit_cast<storage_type*>(
        Bibliotheca::preface_to_corpus(rented_block));
  }

  auto access() const -> const storage_type* {
    return reinterpret_cast<const storage_type*>(
        Bibliotheca::preface_to_corpus(rented_block));
  }

  auto set_capacity(Count required_size) -> void {
    // Check if we can already fit required buffer.
    if (required_size <= capacity) {
      size = required_size;
      return;
    }

    // Attempt to grow by a factor of 2.
    // If that doesn't work than grow to exact size.
    const new_capacity = std::max(capacity * 2, required_size);

    // Fetch and transfer to new block.
    auto new_block = std::bit_cast<storage_type*>(
        Bibliotheca::check_out(new_capacity * sizeof(storage_type)));
    std::memcpy(new_block, rented_block, size * sizeof(storage_type));
    Bibliotheca::remit(rented_block);

    // Update block and get the new capacity.
    rented_block = new_block;
    capacity = rented_block->usable_bytes() / sizeof(storage_type);
  }

 private:
  Bibliotheca::Preface* rented_block;
  Count capacity;
};

}  // namespace Perimortem::Memory::Allocator
