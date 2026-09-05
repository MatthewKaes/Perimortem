// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/view.hpp"

#include "tetrodotoxin/library/builtin/view/is_empty.hpp"
#include "tetrodotoxin/library/builtin/view/size.hpp"
#include "tetrodotoxin/library/builtin/view/slice.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/model/types/unsigned.hpp"
#include "ttx/concept/unknown.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library::Language;

Types::View::View(
    Perimortem::Memory::Allocator::Arena& domain,
    Perimortem::Core::View::Bytes name,
    const Model::Type& element,
    const Model::Type& size_type,
    const Model::Type& flag_type)
    : name(name), element(element), admission(*this) {
  auto& get_size = Builtin::View::Size::create(domain, *this, size_type);
  auto& is_empty = Builtin::View::IsEmpty::create(domain, *this, flag_type);
  auto& slice = Builtin::View::Slice::create(domain, *this, size_type, *this);
  publish_callable(domain, get_size, True);
  publish_callable(domain, is_empty, True);
  publish_callable(domain, slice, True);
}

auto Types::View::resolve_concept(Perimortem::Core::View::Bytes route) const
    -> const Abstract& {
  return route == "admission"_view ? admission
                                   : Contiguous::resolve_concept(route);
}

void Types::View::visit_concepts(ttx_named_abstract_callable* visitor) const {
  Contiguous::visit_concepts(visitor);
  visit_concept(visitor, "admission"_view, admission);
}

auto Types::View::initialize_default(
    Perimortem::Memory::Allocator::Arena& arena) const -> Option<Model::Pack&> {
  return Constants::Bytes::create_synthetic(arena, *this, {});
}

auto Types::View::accepts(const Model::Pack& source) const -> Bool {
  if (source.get_layout().get_size() != 1) {
    return False;
  }

  auto source_view = source.get_value_type(0).resolve().select<Types::View>();
  auto target_element = element.select<Model::Types::Unsigned>();
  auto source_element =
      source_view
          ? source_view->get_element_type().select<Model::Types::Unsigned>()
          : Option<const Model::Types::Unsigned&>();
  return source_view && target_element && source_element &&
         target_element->get_width() == 8 && source_element->get_width() == 8;
}
