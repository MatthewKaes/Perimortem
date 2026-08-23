// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/option.hpp"

#include "tetrodotoxin/library/language/constants/option.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library::Language;

static auto select_option_constant(Model::Pack& source)
    -> Option<Constants::Option&> {
  auto direct = source.select<Constants::Option>();
  if (direct) {
    return *direct;
  }

  const Ttx::Concept::Layout& layout = source.get_layout();
  BAIL_IF(layout.get_size() != 1);
  return layout.get_abstract(0).visit(
      []() -> Option<Constants::Option&> { return {}; },
      [](const Ttx::Concept::Abstract& selected) -> Option<Constants::Option&> {
        auto pack =
            const_cast<Ttx::Concept::Abstract&>(selected).select<Model::Pack>();
        return pack ? pack->select<Constants::Option>()
                    : Option<Constants::Option&>();
      });
}

auto Types::Option::create_default(Perimortem::Memory::Allocator::Arena& arena)
    const -> Perimortem::Core::Option<Model::Pack&> {
  return Constants::Option::create_absent(arena, *this);
}

auto Types::Option::fold_propagation(Model::Pack& source) const -> Perimortem::
    Utility::Result<Perimortem::Core::Option<Model::Pack&>, Bool> {
  auto selected = select_option_constant(source);
  if (!selected || &selected->get_type() != this) {
    return False;
  }

  auto payload = selected->get_payload();
  return payload ? Perimortem::Core::Option<Model::Pack&>(
                       const_cast<Model::Pack&>(*payload))
                 : Perimortem::Core::Option<Model::Pack&>();
}

auto Types::Option::accepts(const Model::Pack& source) const -> Bool {
  return source.get_layout().is_empty() || source.fits_into(element);
}

auto Types::Option::create_fitted(
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
