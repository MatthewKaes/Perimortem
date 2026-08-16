// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/environment/dialects.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;

Environment::Dialects::Dialects(Allocator::Arena& arena)
    : arena(arena), instances(arena) {}

Environment::Dialects::~Dialects() {
  // Workspace member order has already released every retained Monograph.
  for (Count i = instances.get_size(); i > 0; i--) {
    instances[i - 1]->~Dialect();
  }
}

auto Environment::Dialects::contains_name(View::Bytes name) const -> Bool {
  return instances.get_view().contains([&](const Language::Dialect* dialect) {
    return dialect->get_name() == name;
  });
}

auto Environment::Dialects::contains_instance(
    const Language::Dialect& dialect) const -> Bool {
  return instances.get_view().contains([&](const Language::Dialect* candidate) {
    return candidate == &dialect;
  });
}

auto Environment::Dialects::get_dialects() const
    -> View::Vector<Language::Dialect*> {
  return instances.get_view();
}
