// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/constant.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library;

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
