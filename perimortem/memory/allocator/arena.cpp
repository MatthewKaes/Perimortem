// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/math.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory::Allocator;

Arena::Arena() {
  rented_block = nullptr;

  fetch_page(page_size);
}

Arena::~Arena() {
  while (rented_block != nullptr) {
    auto rented = rented_block;
    rented_block = *Core::Data::cast<Bits_8*>(rented_block);
    Bibliotheca::remit(rented);
  }
}

auto Arena::reset() -> void {
  // Return all blocks we've rented from the Bibliotheca until we only have a
  // single block left.
  Bits_8* previous = *Core::Data::cast<Bits_8*>(rented_block);

  while (previous != nullptr) {
    auto rented = rented_block;
    rented_block = previous;
    Bibliotheca::remit(rented);

    previous = *Core::Data::cast<Bits_8*>(rented_block);
  }

  // Reset our usage since we are reusing the last block.
  usage = sizeof(Bits_8*);
}

auto Arena::fetch_page(Count bytes_requested) -> void {
  const Count alloc_size =
      Math::max(page_size, bytes_requested + sizeof(Bits_8*));
  auto alloc = Core::Bibliotheca::check_out(alloc_size);

  // Store the previous pointer in the arena itself.
  Bits_8** previous = Core::Data::cast<Bits_8*>(alloc.ptr);

  // Store and swap the blocks.
  *previous = rented_block;
  rented_block = alloc.ptr;

  // Bump the usage so we don't overwrite the old block.
  usage = sizeof(Bits_8*);
}
