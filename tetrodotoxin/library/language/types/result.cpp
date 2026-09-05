// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/result.hpp"

#include "tetrodotoxin/library/language/constants/result.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library::Language;

auto Types::Result::resolve_concept(Core::View::Bytes route) const
    -> const Ttx::Concept::Abstract& {
  return route == "propagation"_view      ? propagation
         : route == "admission"_view      ? admission
         : route == "initialization"_view ? initialization
                                          : Model::Type::resolve_concept(route);
}

void Types::Result::visit_concepts(ttx_named_abstract_callable* visitor) const {
  Model::Type::visit_concepts(visitor);
  visit_concept(visitor, "propagation"_view, propagation);
  visit_concept(visitor, "admission"_view, admission);
  visit_concept(visitor, "initialization"_view, initialization);
}

auto Types::Result::initialize_default(Memory::Allocator::Arena& arena) const
    -> Core::Option<Model::Pack&> {
  auto selected = Model::initialize_default(value, arena);
  BAIL_IF(!selected);
  auto created = Constants::Result::create_value(arena, *this, *selected);
  return created ? Core::Option<Model::Pack&>(*created)
                 : Core::Option<Model::Pack&>();
}

auto Types::Result::accepts(const Model::Pack& source) const -> Bool {
  if (source.fits(*this)) {
    return True;
  }

  Bool accepts_value = source.fits_into(value);
  Bool accepts_error = source.fits_into(error);
  return accepts_value != accepts_error;
}

auto Types::Result::create_admitted(
    Memory::Allocator::Arena& arena,
    Model::Pack& source) const -> Core::Option<Model::Pack&> {
  auto fitted = Constants::Result::create_fitted(arena, *this, source);
  return fitted ? Core::Option<Model::Pack&>(*fitted)
                : Core::Option<Model::Pack&>();
}

auto Types::Result::validate_layout(Ttx::Lexical::Cursor& cursor) const
    -> Bool {
  if (!value.get_layout().is_empty() && !error.get_layout().is_empty()) {
    return True;
  }

  cursor.create_error(
      "Result alternatives must each produce one nonempty value Type."_view,
      "Replace the empty value or error Type before using this Result."_view);
  return False;
}
