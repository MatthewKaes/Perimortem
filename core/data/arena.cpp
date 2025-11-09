// Perimortem Engine
// Copyright © Matt Kaes

#include "core/data/arena.hpp"

using namespace Perimortem::Concepts;
using namespace Perimortem::Data;

Arena::Arena() {
  rented_block = Bibliotheca::check_out(page_size);
}

Arena::~Arena() {
  while (rented_block != nullptr) {
    auto rented = rented_block;
    rented_block = rented_block->previous;
    Bibliotheca::remit(rented);
  }
}

auto Arena::allocate(uint16_t bytes_requested, uint8_t alignment) -> uint8_t* {
#if PERI_DEBUG
  // You can't request more data than the size of a page.
  if (bytes_requested > page_size) {
    __builtin_debugtrap();
  }
#endif

  auto required_alignment = alignment - (rented_block->usage & (alignment - 1));

  // Fetch a new page if we are full,
  if (rented_block->usage + bytes_requested + required_alignment >
      rented_block->capacity) {
    auto rent = Bibliotheca::check_out(page_size);

    // Add the block to our our stack and make it top most block.
    rent->previous = rented_block;
    rented_block = rent;
  } else {
    // The current page may not be correctly aligned, so apply alignment.
    rented_block->usage += alignment - (rented_block->usage & (alignment - 1));
  }

  uint8_t* root = Bibliotheca::preface_to_corpus(rented_block);
  rented_block->usage += bytes_requested;
  return root;
}

auto Arena::reset() -> void {
  // Return all blocks we've rented from the Bibliotheca until we only have a
  // single block left.
  while (rented_block->previous != nullptr) {
    auto rented = rented_block;
    rented_block = rented_block->previous;
    Bibliotheca::remit(rented);
  }

  // Do a slight optimization by reusing the last block by simply resetting
  // it's usage flag.
  rented_block->usage = sizeof(Bibliotheca::Preface);
}
