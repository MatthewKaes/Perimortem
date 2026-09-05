// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/contiguous.hpp"

#include "ttx/ffi/cpp/addressable.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library;

auto Language::Types::Contiguous::resolve_concept(View::Bytes route) const
    -> const Abstract& {
  return route == "iteration"_view        ? iteration
         : route == "initialization"_view ? initialization
                                          : Model::Type::resolve_concept(route);
}

void Language::Types::Contiguous::visit_concepts(
    ttx_named_abstract_callable* visitor) const {
  Model::Type::visit_concepts(visitor);
  visit_concept(visitor, "iteration"_view, iteration);
  visit_concept(visitor, "initialization"_view, initialization);
}

auto Language::Types::Contiguous::accepts_binding(const Layout& bindings) const
    -> Bool {
  auto binding = bindings.get_abstract(0);
  auto addressable = binding ? binding->select<Ttx::Model::Addressable>()
                             : Option<const Ttx::Model::Addressable&>();
  return bindings.get_size() == 1 && addressable &&
         &addressable->get_domain().resolve() == &get_element_type().resolve();
}
