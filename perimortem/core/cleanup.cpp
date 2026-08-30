// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/cleanup.hpp"

#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/data.hpp"

using namespace Perimortem;

static auto get_cleanup_inventory() -> Core::Cleanup& {
  thread_local Core::Cleanup inventory;
  return inventory;
}

Core::Cleanup::~Cleanup() {
  while (latest) {
    Entry& selected = *latest;
    latest = selected.previous;
    selected.destructor();
    Core::Bibliotheca::remit(Core::Data::cast<U8>(&selected));
  }
}

auto Core::Cleanup::insert(Destructor destructor) -> void {
  Core::Bibliotheca::Allocation allocation =
      Core::Bibliotheca::check_out(sizeof(Entry));
  Entry* entry = Core::Data::cast<Entry>(allocation.ptr);
  new (entry, Core::Placement::Construct) Entry(destructor, latest);
  latest = *entry;
}

extern "C" auto perimortem_core_cleanup_register(
    Core::Cleanup::Destructor destructor) -> void {
  Core::Bibliotheca::Allocation order = Core::Bibliotheca::check_out(1);
  Core::Bibliotheca::remit(order.ptr);
  get_cleanup_inventory().insert(destructor);
}
