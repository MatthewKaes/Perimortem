// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/parameter.hpp"

using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library;

auto Language::Parameter::create_authored(
    Perimortem::Memory::Allocator::Arena& domain,
    const Perimortem::Core::View::Bytes& name,
    const Ttx::Model::Type& type) -> Perimortem::Core::Option<Parameter&> {
  if (name.is_empty() || type.get_layout().is_empty()) {
    return {};
  }

  return domain.construct_from<Parameter>(
      [&]() -> Parameter { return Parameter(name, type); });
}

auto Language::Parameter::get_documentation() const -> const Documentation& {
  return Documentation::get_empty();
}

auto Language::Parameter::get_type() const -> const Ttx::Model::Type& {
  return type;
}
