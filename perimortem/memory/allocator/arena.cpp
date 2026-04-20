// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/memory/allocator/arena.hpp"

using namespace Perimortem::Memory::Allocator;

Arena::Arena() {
  rented_block = Bibliotheca::check_out(page_size);
  usage = 0;
}

Arena::Arena(Arena&& arena) {}

Arena::~Arena() {
  while (rented_block != nullptr) {
    auto rented = rented_block;
    rented_block = *reinterpret_cast<Bibliotheca::Preface**>(
        Bibliotheca::preface_to_corpus(rented_block));
    Bibliotheca::remit(rented);
  }
}

auto Arena::reset() -> void {
  // Return all blocks we've rented from the Bibliotheca until we only have a
  // single block left.
  Bibliotheca::Preface** previous = reinterpret_cast<Bibliotheca::Preface**>(
      Bibliotheca::preface_to_corpus(rented_block));

  while (previous != nullptr) {
    auto rented = rented_block;
    rented_block = *previous;
    Bibliotheca::remit(rented);

    previous = reinterpret_cast<Bibliotheca::Preface**>(
        Bibliotheca::preface_to_corpus(rented_block));
  }

  // Reset our usage since we are reusing the last block.
  usage = sizeof(Bibliotheca::Preface);
}
