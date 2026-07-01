// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/dialects/dialect.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Dialect;

static Static::Vector<Ttx::Dialect*, 64> binary_tests;
static Count registered_dialects = 0;

Registry::Registration::Registration(const Dialect& dialect) {
  Registry::add(dialect);
}

auto Registry::add(const Dialect& dialect) -> void {
  Dynamic::Vector<const Dialect*>& dialects = registered_dialects();
  for (Count dialect_index = 0; dialect_index < dialects.get_size();
       dialect_index++) {
    if (dialects[dialect_index] == &dialect ||
        dialects[dialect_index]->get_name() == dialect.get_name()) {
      return;
    }
  }

  dialects.insert(&dialect);
}

auto Registry::find(View::Bytes name) -> const Dialect* {
  Dynamic::Vector<const Dialect*>& dialects = registered_dialects();
  for (Count dialect_index = 0; dialect_index < dialects.get_size();
       dialect_index++) {
    if (dialects[dialect_index]->get_name() == name) {
      return dialects[dialect_index];
    }
  }

  return nullptr;
}

auto Registry::get_dialects() -> View::Vector<const Dialect*> {
  return registered_dialects().get_view();
}
