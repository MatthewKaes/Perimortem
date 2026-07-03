// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/boot/selection.hpp"

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
  message.append("ISA `"_view);
  message.append(requested);
  message.append("` is not installed."_view);

  auto installed = registry.get_installed();
  if (installed.is_empty()) {
    message.append(" No ISAs are installed."_view);
    return message.get_view();
  }

  message.append(" Installed ISAs: "_view);
  for (Count isa_index = 0; isa_index < installed.get_size(); isa_index++) {
    if (isa_index != 0) {
      message.append(", "_view);
    }
    message.append(installed[isa_index].get_name());
  }
  message.append("."_view);
  return message.get_view();
}

auto Selection::evaluate(Cursor& cursor, const Registry& registry)
    -> Selection {
  if (!cursor.require(
          Class::Type::Define,
          "Expected `:` after dialect instruction."_view)) {
    return Selection();
  }

  const Token* name =
      cursor.require(Class::Type::Type, "Expected ISA name."_view);
  if (name == nullptr) {
    return Selection();
  }

  if (registry.find(name->get_text()) == nullptr) {
    cursor.range_error(
        *name, *name,
        installed_isa_message(cursor, name->get_text(), registry));
    return Selection();
  }

  return Selection(name->get_text());
}
