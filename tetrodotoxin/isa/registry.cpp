// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/registry.hpp"

#include "perimortem/memory/managed/bytes.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

static auto installed_isa_message(
    Cursor& cursor,
    View::Bytes requested,
    const Registry& registry) -> View::Bytes {
  Managed::Bytes message(cursor.get_arena());
  message.concat("ISA `"_view);
  message.concat(requested);
  message.concat("` is not installed."_view);

  auto installed = registry.get_installed();
  if (installed.is_empty()) {
    message.concat(" No ISAs are installed."_view);
    return message.get_view();
  }

  message.concat(" Installed ISAs: "_view);
  for (Count i = 0; i < installed.get_size(); i++) {
    if (i != 0) {
      message.concat(", "_view);
    }
    message.concat(installed[i].get_name());
  }
  message.concat("."_view);
  return message.get_view();
}

auto Registry::install(
    View::Bytes name,
    EvaluateFunction evaluator) -> Bool {
  Entry entry(name, evaluator);
  if (!entry.is_valid() || installed_count >= installed.get_size()) {
    return False;
  }

  for (Count i = 0; i < installed_count; i++) {
    if (installed[i].get_name() == name) {
      installed[i] = entry;
      return True;
    }
  }

  installed[installed_count] = entry;
  installed_count++;
  return True;
}

auto Registry::find(View::Bytes name) const -> const Entry* {
  for (Count i = 0; i < installed_count; i++) {
    if (installed[i].get_name() == name) {
      return installed.get_data() + i;
    }
  }

  return nullptr;
}

auto Registry::require_installed(Cursor& cursor, const Token& name) const
    -> Bool {
  if (find(name.get_text()) != nullptr) {
    return True;
  }

  cursor.range_error(
      name, name, installed_isa_message(cursor, name.get_text(), *this));
  return False;
}

auto Registry::require_installed(Cursor& cursor, View::Bytes name) const
    -> Bool {
  if (find(name) != nullptr) {
    return True;
  }

  cursor.error(installed_isa_message(cursor, name, *this));
  return False;
}

auto Registry::get_installed() const
    -> Perimortem::Core::View::Vector<Entry> {
  return {installed.get_data(), installed_count};
}
