// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/bootstrap/model/layouts/termination.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "ttx/bootstrap/model/addressable.hpp"

using namespace Perimortem;
using namespace Ttx;

static auto is_terminating(
    const Model::Type& type,
    Memory::Dynamic::Vector<const Model::Type*>& active) -> Bool {
  BAIL_IF(active.contains(&type));
  active.insert(&type);

  const Concept::Layout& layout = type.get_layout();
  if (layout.is_empty()) {
    active.remove(active.get_size() - 1);
    return False;
  }
  for (Count index = 0; index < layout.get_size(); index++) {
    auto entry = layout.get_abstract(index);
    BAIL_IF(!entry);

    // An atomic Type names itself as its one terminal Value leaf. A structural
    // edge that eventually returns to the same identity instead reaches the
    // active path below and proves that its shape never ends.
    if (&*entry == &type) {
      continue;
    }

    const Concept::Abstract& resolved = entry->resolve();
    auto child = resolved.select<Model::Type>();
    if (!child) {
      auto addressable = resolved.select<Model::Addressable>();
      BAIL_IF(!addressable);
      child = addressable->get_type().select<Model::Type>();
      BAIL_IF(!child);
    }

    if (!is_terminating(*child, active)) {
      active.remove(active.get_size() - 1);
      return False;
    }
  }

  active.remove(active.get_size() - 1);
  return True;
}

auto Model::Layouts::is_terminating(const Type& type) -> Bool {
  Memory::Dynamic::Vector<const Type*> active;
  return ::is_terminating(type, active);
}
