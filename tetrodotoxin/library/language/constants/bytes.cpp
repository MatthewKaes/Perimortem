// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/constants/bytes.hpp"

#include "perimortem/core/static/vector.hpp"

#include "ttx/concept/none.hpp"
#include "ttx/reference/model/layouts/fluid.hpp"
#include "ttx/reference/model/layouts/named.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library::Language;

auto Constants::Bytes::resolve_concept(Core::View::Bytes name) const
    -> const Abstract& {
  if (name != "resource"_view) {
    return None::get_none();
  }
  return resource.visit(
      []() -> const Abstract& { return None::get_none(); },
      [](const Tetrodotoxin::Language::Resource* selected) -> const Abstract& {
        return *selected;
      });
}

auto Constants::Bytes::visit_concepts(
    ttx_named_abstract_callable* visitor) const -> void {
  Tetrodotoxin::Library::Language::Constant::visit_concepts(visitor);
  auto selected = get_resource();
  if (selected) {
    visit_concept(visitor, "resource"_view, *selected);
  }
}
