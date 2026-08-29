// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/constants/bytes.hpp"

#include "perimortem/core/static/vector.hpp"

#include "ttx/concept/none.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/layouts/named.hpp"

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
      [](const Reference<const Tetrodotoxin::Language::Resource>& selected)
          -> const Abstract& { return selected.get(); });
}

auto Constants::Bytes::get_concepts(Ttx::Concept::Context& context) const
    -> const Ttx::Concept::Pack& {
  auto selected = get_resource();
  if (!selected) {
    return Tetrodotoxin::Library::Language::Constant::get_concepts(context);
  }
  Core::Static::Vector<Reference<const Abstract>, 1> values = {{
    *selected,
  }};
  Ttx::Model::Layouts::Fluid fluid(values);
  static constexpr Core::Static::Vector<Core::View::Bytes, 1> names = {{
    "resource"_view,
  }};
  Ttx::Model::Layouts::Named named(fluid, names);
  return context.pack(named);
}
