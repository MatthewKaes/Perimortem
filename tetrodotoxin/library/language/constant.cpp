// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/constant.hpp"

#include "ttx/concept/unknown.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library;

auto Language::Constant::resolve_concept(Core::View::Bytes route) const
    -> const Ttx::Concept::Abstract& {
  return route == "fold"_view ? static_cast<const Abstract&>(*this)
                              : Ttx::Concept::Constant::resolve_concept(route);
}

auto Language::Constant::visit_concepts(
    ttx_named_abstract_callable* visitor) const -> void {
  Ttx::Concept::Constant::visit_concepts(visitor);
  visit_concept(visitor, "fold"_view, *this);
}

void Language::Constant::domain(ttx_abstract, ttx_domain_result result) const {
  const ttx_abstract selected = get_type().get_handle();
  selected.operations->resolve_domain(selected, result);
}

auto Language::Constant::get_value_type(Count index) const
    -> const Ttx::Concept::Abstract& {
  return index == 0 ? static_cast<const Ttx::Concept::Abstract&>(get_type())
                    : static_cast<const Ttx::Concept::Abstract&>(
                          Ttx::Concept::Unknown::get_unknown());
}

auto Language::Constant::have_equal_values(
    const Model::Pack& left,
    const Model::Pack& right) -> Bool {
  const Layout& left_layout = left.get_layout();
  const Layout& right_layout = right.get_layout();
  if (left_layout.get_size() != right_layout.get_size()) {
    return False;
  }

  for (Count index = 0; index < left_layout.get_size(); index++) {
    auto left_entry = left_layout.get_abstract(index);
    auto right_entry = right_layout.get_abstract(index);
    auto left_constant = left_entry.visit(
        []() -> Core::Option<const Language::Constant&> { return {}; },
        [](const Abstract& selected) {
          return selected.select<Language::Constant>();
        });
    auto right_constant = right_entry.visit(
        []() -> Core::Option<const Language::Constant&> { return {}; },
        [](const Abstract& selected) {
          return selected.select<Language::Constant>();
        });
    if (!left_constant || !right_constant ||
        *left_constant != *right_constant) {
      return False;
    }
  }

  return True;
}
