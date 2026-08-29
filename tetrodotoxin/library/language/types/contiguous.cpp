// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/contiguous.hpp"

#include "ttx/bootstrap/model/addressable.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library;

auto Language::Types::Contiguous::accepts_iteration(
    const Layout& bindings) const -> Bool {
  auto binding = bindings.get_abstract(0);
  auto addressable = binding ? binding->select<Ttx::Model::Addressable>()
                             : Option<const Ttx::Model::Addressable&>();
  return bindings.get_size() == 1 && addressable &&
         &addressable->get_type().resolve() == &get_element_type().resolve();
}
