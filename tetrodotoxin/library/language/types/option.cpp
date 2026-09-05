// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/option.hpp"

#include "tetrodotoxin/library/language/constants/option.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library::Language;

auto Types::Option::resolve_concept(View::Bytes route) const
    -> const Ttx::Concept::Abstract& {
  return route == "propagation"_view      ? propagation
         : route == "admission"_view      ? admission
         : route == "initialization"_view ? initialization
                                          : Model::Type::resolve_concept(route);
}

void Types::Option::visit_concepts(ttx_named_abstract_callable* visitor) const {
  Model::Type::visit_concepts(visitor);
  visit_concept(visitor, "propagation"_view, propagation);
  visit_concept(visitor, "admission"_view, admission);
  visit_concept(visitor, "initialization"_view, initialization);
}

auto Types::Option::initialize_default(
    Perimortem::Memory::Allocator::Arena& arena) const
    -> Perimortem::Core::Option<Model::Pack&> {
  return Constants::Option::create_absent(arena, *this);
}

auto Types::Option::accepts(const Model::Pack& source) const -> Bool {
  return source.get_layout().is_empty() || source.fits_into(element);
}

auto Types::Option::create_admitted(
    Perimortem::Memory::Allocator::Arena& arena,
    Model::Pack& source) const -> Perimortem::Core::Option<Model::Pack&> {
  auto fitted = Constants::Option::create_fitted(arena, *this, source);
  return fitted ? Perimortem::Core::Option<Model::Pack&>(*fitted)
                : Perimortem::Core::Option<Model::Pack&>();
}

auto Types::Option::validate_layout(Ttx::Lexical::Cursor& cursor) const
    -> Bool {
  if (!element.get_layout().is_empty()) {
    return True;
  }

  cursor.create_error(
      "Option requires one nonempty element Type."_view,
      "Replace the empty element before using this Option."_view);
  return False;
}
