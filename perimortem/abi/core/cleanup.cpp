// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/abi/core/cleanup.hpp"

#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/data.hpp"

using namespace Perimortem;

Abi::Core::Cleanup::~Cleanup() {
  while (latest) {
    Entry& selected = *latest;
    latest = selected.previous;
    selected.destructor();
    Perimortem::Core::Bibliotheca::remit(
        Perimortem::Core::Data::cast<Unsigned_8>(&selected));
  }
}

auto Abi::Core::Cleanup::insert(Destructor destructor) -> void {
  Perimortem::Core::Bibliotheca::Allocation allocation =
      Perimortem::Core::Bibliotheca::check_out(sizeof(Entry));
  Entry* entry = Perimortem::Core::Data::cast<Entry>(allocation.ptr);
  new (entry) Entry(destructor, latest);
  latest = *entry;
}
