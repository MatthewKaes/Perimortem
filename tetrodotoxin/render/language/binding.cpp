// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/render/language/binding.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Render;

auto Language::Binding::create_authored(
    Perimortem::Memory::Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition,
    Kind kind,
    Tetrodotoxin::Language::TypeReference type) -> Binding& {
  return domain.construct_from<Binding>([&]() {
    return Binding(
        definition.get_name(), definition, kind, type,
        Option<Reference<const Ttx::Model::Type>>());
  });
}

auto Language::Binding::create_slot(
    Perimortem::Memory::Allocator::Arena& domain,
    View::Bytes name,
    const Ttx::Model::Type& type) -> Binding& {
  return domain.construct_from<Binding>([&]() {
    return Binding(
        name, {}, Kind::Parameter, {}, Reference<const Ttx::Model::Type>(type));
  });
}

auto Language::Binding::link(Cursor& cursor, const Abstract& context) -> Bool {
  if (type) {
    return True;
  }
  BAIL_IF(!type_reference);
  auto selected = type_reference->resolve(cursor, context);
  BAIL_IF(!selected || selected->get_layout().is_empty());
  type = Reference<const Ttx::Model::Type>(*selected);
  return True;
}

auto Language::Binding::get_documentation() const -> const Documentation& {
  return definition.visit(
      []() -> const Documentation& { return Documentation::get_empty(); },
      [](const Tetrodotoxin::Language::Definition& selected)
          -> const Documentation& { return selected.get_documentation(); });
}

auto Language::Binding::get_type() const -> const Ttx::Model::Type& {
  return type->get();
}

auto Language::Binding::resolve() const -> const Abstract& {
  return type ? static_cast<const Abstract&>(*this)
              : static_cast<const Abstract&>(Invalid::get_invalid());
}
