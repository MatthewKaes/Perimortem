// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/memory/dynamic/record.hpp"

using namespace Perimortem::Memory::Dynamic;
using namespace Perimortem::Memory::Allocator;

Record::Record() : reserved_block(nullptr) {};
Record::Record(Count size) {
  reserved_block = Bibliotheca::check_out(size);
  *reinterpret_cast<Count*>(Bibliotheca::preface_to_corpus(reserved_block)) =
      size;
};

Record::Record(const Record& rhs) {
  Bibliotheca::reserve(rhs.reserved_block);
  reserved_block = rhs.reserved_block;
}

auto Record::operator=(Record&& rhs) -> Record& {
  // If we have a valid Record at least one reference will be lost.
  if (reserved_block) {
    // Remit this block since it covers both cases:
    //
    // If it's the same reference in both Records then it doesn't matter which
    // one we remit.
    //
    // If they are different then this block is the one that should be
    // remitted anyway.
    Bibliotheca::remit(reserved_block);
  }

  reserved_block = rhs.reserved_block;
  rhs.reserved_block = nullptr;

  return *this;
}

auto Record::operator=(const Record& rhs) -> Record& {
  if (!reserved_block) {
    Bibliotheca::reserve(rhs.reserved_block);
  } else {
    Bibliotheca::exchange(reserved_block, rhs.reserved_block);
  }

  reserved_block = rhs.reserved_block;
  return *this;
}

auto Record::operator=(View::Bytes data) -> Record& {
  set_size(data.get_size(), false);

  if (reserved_block) {
    std::memcpy(get_data(), data.get_data(), data.get_size());
  }

  return *this;
}

Record::~Record() {
  clear();
}

auto Record::clear() -> void {
  if (!reserved_block) {
    return;
  }

  Bibliotheca::remit(reserved_block);
  reserved_block = nullptr;
}

auto Record::is_valid() const -> bool {
  return !reserved_block;
}

auto Record::get_view() const -> View::Bytes {
  if (!reserved_block)
    return View::Bytes();

  return View::Bytes(Bibliotheca::preface_to_corpus(reserved_block),
                     get_size());
}

auto Record::get_buffer() -> Managed::Buffer {
  if (!reserved_block)
    return Managed::Buffer();

  // Fork on multi-ownership since the data being mutated is expected.
  if (reserved_block->get_reservations() != 1) {
    auto original_size = get_size();
    auto original_block = reserved_block;
    reserved_block = Bibliotheca::check_out(get_size());

    // Block copy everything in the buffer (including size data)
    std::memcpy(Bibliotheca::preface_to_corpus(reserved_block),
                Bibliotheca::preface_to_corpus(original_block), original_size);
    Bibliotheca::remit(original_block);
  }

  return Managed::Buffer(Bibliotheca::preface_to_corpus(reserved_block),
                         get_size());
}

auto Record::get_size() const -> Count {
  if (!reserved_block) {
    return 0;
  }

  return *reinterpret_cast<Count*>(
      Bibliotheca::preface_to_corpus(reserved_block));
}

auto Record::get_capacity() const -> Count {
  if (!reserved_block) {
    return 0;
  }

  return reserved_block->usable_bytes() - sizeof(Count);
}

auto Record::outstanding_copies() const -> Count {
  if (!reserved_block) {
    return 0;
  }

  return reserved_block->get_reservations();
}

auto Record::set_size(Count size, bool copy_data) -> void {
  if (!reserved_block) {
    if (size == 0) {
      return;
    }

    reserved_block = Bibliotheca::check_out(size);
    *reinterpret_cast<Count*>(Bibliotheca::preface_to_corpus(reserved_block)) =
        size;

    std::memset(get_data(), 0, size);
    return;
  }

  // No change.
  if (size == get_size()) {
    return;
  }

  // Clear block.
  if (size == 0) {
    clear();
    return;
  }

  // Grow or shrink the block.
  auto original_size = get_size();
  if (reserved_block->get_reservations() != 1 || size > get_capacity() ||
      size < get_capacity() / 4) {
    auto original_block = reserved_block;
    reserved_block = Bibliotheca::check_out(size);

    if (copy_data) {
      std::memcpy(
          Bibliotheca::preface_to_corpus(reserved_block) + sizeof(Count),
          Bibliotheca::preface_to_corpus(original_block) + sizeof(Count),
          std::min(size, original_size));
    }
    Bibliotheca::remit(original_block);
  }

  // Set the new size into the buffer post possible fork.
  *reinterpret_cast<Count*>(Bibliotheca::preface_to_corpus(reserved_block)) =
      size;

  if (size > original_size) {
    std::memset(get_data() + original_size, 0, size + original_size);
  }
}