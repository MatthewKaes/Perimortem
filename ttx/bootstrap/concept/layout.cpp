// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/bootstrap/concept/layout.hpp"

#include <stddef.h>

using namespace Perimortem::Core;
using namespace Ttx::Concept;

auto Layout::from_abi(const ttx_layout* layout) -> const Layout& {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
  const Count offset = __builtin_offsetof(Layout, abi);
#pragma clang diagnostic pop
  return *reinterpret_cast<const Layout*>(
      reinterpret_cast<const U8*>(layout) - offset);
}

auto Layout::visit_abi(const ttx_layout* layout, ttx_abstract_callable* visitor)
    -> void {
  const Layout& selected = from_abi(layout);
  for (Count index = 0; index < selected.get_size(); ++index) {
    auto abstract = selected.get_abstract(index);
    if (abstract) {
      ttx_abstract_callable_call(visitor, abstract->get_abi());
    }
  }
}

auto Layout::fits_abi(const ttx_layout* source, const ttx_layout* target)
    -> perimortem_bool {
  return from_abi(source).fits(from_abi(target)) ? PERIMORTEM_TRUE
                                                 : PERIMORTEM_FALSE;
}

const ttx_layout_operations Layout::abi_operations = {
  .visit = visit_abi,
  .fits = fits_abi,
};
