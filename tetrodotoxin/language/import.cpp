// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/import.hpp"

#include "ttx/model/documentations/merged.hpp"

using namespace Ttx::Concept;
using namespace Tetrodotoxin;

auto Language::Import::bind(const Abstract& target) -> Bool {
  BAIL_IF(!bind_target(target));
  const Documentation& target_documentation = target.get_documentation();
  if (local_documentation.is_empty()) {
    visible_documentation = target_documentation;
  } else if (target_documentation.is_empty()) {
    visible_documentation = local_documentation;
  } else {
    visible_documentation =
        domain.construct<Ttx::Model::Documentations::Merged>(
            local_documentation, target_documentation);
  }
  return True;
}

auto Language::Import::get_documentation() const -> const Documentation& {
  return visible_documentation.visit(
      [&]() -> const Documentation& { return local_documentation; },
      [](const Documentation& selected) -> const Documentation& {
        return selected;
      });
}
