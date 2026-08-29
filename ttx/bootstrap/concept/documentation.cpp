// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/bootstrap/concept/documentation.hpp"

#include <stddef.h>

using namespace Perimortem::Core;

auto Ttx::Concept::Documentation::from_abi(
    const ttx_documentation* documentation) -> const Documentation& {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
  const Count offset = __builtin_offsetof(Ttx::Concept::Documentation, abi);
#pragma clang diagnostic pop
  return *reinterpret_cast<const Ttx::Concept::Documentation*>(
      reinterpret_cast<const U8*>(documentation) - offset);
}

const ttx_documentation_operations Ttx::Concept::Documentation::abi_operations =
    {
      .visit = visit_abi,
};

auto Ttx::Concept::Documentation::visit_abi(
    const ttx_documentation* documentation,
    ttx_bytes_callable* visitor) -> void {
  const Documentation& selected = from_abi(documentation);
  for (Count index = 0; index < selected.line_count(); ++index) {
    View::Bytes line = selected.get_line(index);
    ttx_bytes_callable_call(
        visitor, perimortem_bytes{
                   .data = line.get_data(),
                   .size = line.get_size(),
                 });
  }
}

auto Ttx::Concept::Documentation::get_empty() -> const Documentation& {
  static constexpr Documentation documentation;
  return documentation;
}
