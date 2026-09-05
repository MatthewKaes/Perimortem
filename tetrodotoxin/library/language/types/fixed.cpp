// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/fixed.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/builtin/fixed/access.hpp"
#include "tetrodotoxin/library/builtin/fixed/view.hpp"
#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/expressions/initializer.hpp"
#include "tetrodotoxin/library/language/fold.hpp"
#include "ttx/concept/unknown.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library::Language;

Types::Fixed::Fixed(
    Allocator::Arena& domain,
    View::Bytes name,
    const Model::Type& element,
    ::U64 extent,
    const Model::Type& access_type,
    const Model::Type& view_type)
    : name(name),
      element(element),
      extent(extent),
      layout(element, Count(extent)),
      admission(*this) {
  auto& get_access = Builtin::Fixed::Access::create(domain, *this, access_type);
  auto& get_view = Builtin::Fixed::View::create(domain, *this, view_type);
  publish_callable(domain, get_access, True);
  publish_callable(domain, get_view, True);
}

auto Types::Fixed::initialize_default(Allocator::Arena& arena) const
    -> Option<Model::Pack&> {
  BAIL_IF(get_extent() == 0 || get_extent() > U64(Count(-1)));

  Managed::Vector<Model::Pack*> values(arena);
  values.reset(Count(get_extent()));
  for (Count index = 0; index < Count(get_extent()); index++) {
    auto value = Model::initialize_default(get_element_type(), arena);
    BAIL_IF(!value);
    values.insert(&*value);
  }

  return Expressions::Initializer::create_synthetic(
      arena, *this, values.get_view());
}

static auto fold_output(Model::Pack& source, Count index)
    -> Option<Model::Pack&> {
  auto producer = source.get_layout().get_abstract(index);
  BAIL_IF(!producer);
  auto direct = const_cast<Abstract&>(*producer)
                    .select<Tetrodotoxin::Library::Language::Constant>();
  if (direct) {
    return *direct;
  }

  auto producer_pack = Model::Pack::from(const_cast<Abstract&>(*producer));
  BAIL_IF(!producer_pack);
  auto folded = query_folded_pack(*producer_pack);
  BAIL_IF(!folded);

  Count selected_index = 0;
  for (Count source_index = 0; source_index < index; source_index++) {
    auto prior = source.get_layout().get_abstract(source_index);
    selected_index += prior && &*prior == &*producer ? 1 : 0;
  }
  auto selected = folded->get_layout().get_abstract(selected_index);
  BAIL_IF(!selected);
  auto constant = selected->select<Tetrodotoxin::Library::Language::Constant>();
  BAIL_IF(!constant);
  return const_cast<Tetrodotoxin::Library::Language::Constant&>(*constant);
}

static auto create_bytes(
    Allocator::Arena& arena,
    const Types::Fixed& type,
    View::Vector<Model::Pack*> values) -> Option<Model::Pack&> {
  auto element =
      type.get_element_type().resolve().select<Model::Types::Unsigned>();
  BAIL_IF(!element || element->get_size() != 1);

  auto storage = arena.allocate(values.get_size());
  Count index = 0;
  for (Model::Pack* selected : values) {
    auto value = selected->select_identity<Constants::Unsigned>();
    BAIL_IF(!value || value->get_value() > U64(U8(-1)));
    storage.get_data()[index] = U8(value->get_value());
    index++;
  }

  return Constants::Bytes::create_synthetic(
      arena, type, View::Bytes(storage.get_data(), storage.get_size()));
}

auto Types::Fixed::accepts(const Model::Pack& source) const -> Bool {
  return source.fits(*this);
}

auto Types::Fixed::create_admitted(Allocator::Arena& arena, Model::Pack& source)
    const -> Option<Model::Pack&> {
  BAIL_IF(!accepts(source));

  Managed::Vector<Model::Pack*> values(arena);
  values.reset(Count(get_extent()));
  for (Count index = 0; index < Count(get_extent()); index++) {
    auto value = fold_output(source, index);
    BAIL_IF(!value);
    values.insert(&*value);
  }

  auto element = get_element_type().resolve().select<Model::Types::Unsigned>();
  if (element && element->get_size() == 1) {
    return create_bytes(arena, *this, values.get_view());
  }
  return Model::Pack::create_folded(arena, values.get_view());
}

auto Types::Fixed::resolve_concept(View::Bytes route) const -> const Abstract& {
  return route == "admission"_view ? admission
                                   : Contiguous::resolve_concept(route);
}

void Types::Fixed::visit_concepts(ttx_named_abstract_callable* visitor) const {
  Contiguous::visit_concepts(visitor);
  visit_concept(visitor, "admission"_view, admission);
}
