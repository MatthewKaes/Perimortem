// Perimortem Engine
// Copyright © Matt Kaes

#include "memory/arena.hpp"

using namespace Perimortem::Memory;

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
