// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/dialect/dialect.hpp"

auto Ttx::Dialect::Dialect::Registry::dialects()
    -> Perimortem::Core::Static::Vector<const Dialect*, 64>& {
  // Dialects are process-lifetime objects registered during startup and read
  // during source dispatch. The registry table is fixed storage because it owns
  // only non-owning dialect pointers and should not depend on heap lifetime.
  static Perimortem::Core::Static::Vector<const Dialect*, 64> registered;
  return registered;
}

auto Ttx::Dialect::Dialect::Registry::dialect_count() -> Count& {
  static Count count = 0;
  return count;
}

Ttx::Dialect::Dialect::Registry::Registration::Registration(
    const Dialect& dialect) {
  Registry::add(dialect);
}

auto Ttx::Dialect::Dialect::Registry::add(const Dialect& dialect) -> void {
  Perimortem::Core::Static::Vector<const Dialect*, 64>& registered =
      dialects();
  Count& count = dialect_count();

  for (Count dialect_index = 0; dialect_index < count; dialect_index++) {
    if (registered[dialect_index] == &dialect ||
        registered[dialect_index]->get_name() == dialect.get_name()) {
      return;
    }
  }

  if (count >= registered.get_size()) {
    return;
  }

  registered[count] = &dialect;
  count++;
}

auto Ttx::Dialect::Dialect::Registry::find(Perimortem::Core::View::Bytes name)
    -> const Dialect* {
  Perimortem::Core::Static::Vector<const Dialect*, 64>& registered =
      dialects();
  Count count = dialect_count();

  for (Count dialect_index = 0; dialect_index < count; dialect_index++) {
    if (registered[dialect_index]->get_name() == name) {
      return registered[dialect_index];
    }
  }

  return nullptr;
}

auto Ttx::Dialect::Dialect::Registry::get_dialects()
    -> Perimortem::Core::View::Vector<const Dialect*> {
  return Perimortem::Core::View::Vector<const Dialect*>(
      dialects().get_data(), dialect_count());
}
