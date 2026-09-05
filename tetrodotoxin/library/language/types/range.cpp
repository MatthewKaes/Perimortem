// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/range.hpp"

#include "tetrodotoxin/library/language/constants/range.hpp"
#include "ttx/ffi/cpp/addressable.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library::Language;

auto Types::Range::resolve_concept(View::Bytes route) const
    -> const Ttx::Concept::Abstract& {
  return route == "iteration"_view        ? iteration
         : route == "initialization"_view ? initialization
                                          : Model::Type::resolve_concept(route);
}

void Types::Range::visit_concepts(ttx_named_abstract_callable* visitor) const {
  Model::Type::visit_concepts(visitor);
  visit_concept(visitor, "iteration"_view, iteration);
  visit_concept(visitor, "initialization"_view, initialization);
}

auto Types::Range::initialize_default(
    Perimortem::Memory::Allocator::Arena& arena) const -> Option<Model::Pack&> {
  return Constants::Range::create_synthetic(arena, *this);
}

auto Types::Range::accepts_binding(const Ttx::Concept::Layout& bindings) const
    -> Bool {
  auto binding = bindings.get_abstract(0);
  auto addressable = binding ? binding->select<Ttx::Model::Addressable>()
                             : Option<const Ttx::Model::Addressable&>();
  return bindings.get_size() == 1 && addressable &&
         &addressable->get_domain().resolve() == &element.resolve();
}
