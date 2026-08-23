// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/structure.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/expressions/initializer.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "ttx/concept/reference.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library::Language;

auto Types::Structure::create_authored(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition) -> Structure& {
  return domain.construct_from<Structure>(
      [&]() -> Structure { return Structure(domain, definition); });
}

auto Types::Structure::create_restored(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition) -> Structure& {
  return domain.construct_from<Structure>(
      [&]() -> Structure { return Structure(domain, definition, False); });
}

auto Types::Structure::complete_body() -> void {
  complete_field_layout();
}

auto Types::Structure::resolve_context(View::Bytes route) const
    -> const Ttx::Concept::Abstract& {
  return Composite::resolve_context(route);
}

auto Types::Structure::create_default(Allocator::Arena& arena) const
    -> Option<Model::Pack&> {
  BAIL_IF(get_layout().is_empty());

  if (!provides_initialization) {
    auto& arguments = Model::Pack::create_empty(arena);
    return Expressions::Initializer::create_provider(arena, *this, arguments);
  }

  if (creating_default) {
    Perimortem::Core::Diagnostics::Log::Message<256> message(
        Perimortem::Core::Diagnostics::Log::Level::Error,
        Perimortem::Core::Diagnostics::Source());
    message << "Library default construction recursively reentered Type `"_view
            << get_name() << "`."_view;
    return {};
  }

  creating_default = True;
  auto result = [&]() -> Option<Model::Pack&> {
    // Structure owns this exact instance Field inventory, so selecting Field is
    // construction local to the owner rather than a consumer category switch.
    // Authored Layout order is filled from each Field initializer before asking
    // that Field's exact Type for its default.
    Managed::Vector<Ttx::Concept::Reference<Model::Pack>> values(arena);
    values.reset(get_layout().get_size());
    for (const Ttx::Concept::Reference<Ttx::Concept::Abstract>& candidate :
         get_addressables()) {
      auto field = candidate.get().select<Field>();
      if (!field || field->get_writability() != Writability::Internal) {
        continue;
      }

      auto initializer = field->get_initializer();
      if (initializer) {
        values.insert(const_cast<Model::Pack&>(*initializer));
        continue;
      }

      auto value = field->get_type().create_default(arena);
      BAIL_IF(!value);
      values.insert(*value);
    }

    return Expressions::Initializer::create_synthetic(
        arena, *this, values.get_view());
  }();
  creating_default = False;
  return result;
}
