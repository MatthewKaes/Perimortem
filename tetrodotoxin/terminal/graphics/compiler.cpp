// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/graphics/compiler.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/visibility.hpp"
#include "tetrodotoxin/library/language/interfaces/structure.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "ttx/concept/interface.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;

static auto find_configured_type(
    Core::View::Vector<Ttx::Concept::Reference<const Ttx::Model::Type>>
        configured,
    const Ttx::Model::Type& candidate) -> Core::Option<Count> {
  Core::Option<Count> selected;
  for (Count index = 0; index < configured.get_size(); index++) {
    if (&configured.get_data()[index].get().resolve() == &candidate.resolve()) {
      BAIL_IF(selected);
      selected = index;
    }
  }
  return selected;
}

auto Terminal::Graphics::Compiler::compile(
    Memory::Allocator::Arena& arena,
    const Scene::Language::Monograph& scene,
    const Ttx::Model::Type& requirement,
    Core::View::Vector<Ttx::Concept::Reference<const Ttx::Model::Type>>
        configured) const -> Core::Option<Products> {
  BAIL_IF(!scene.is_finalized() || configured.is_empty());

  Library::Language::Interfaces::Structure hosting;
  Memory::Managed::Vector<Products::Hosted> hosted(arena);
  for (const Ttx::Concept::Reference<Ttx::Concept::Abstract>& declaration :
       scene.get_instance().get_addressables()) {
    auto field = declaration.get().select<Library::Language::Field>();
    if (!field ||
        field->get_definition().get_visibility() !=
            Language::Visibility::Private ||
        field->get_writability() != Library::Language::Writability::Internal) {
      continue;
    }

    auto object = field->get_type().select<Library::Language::Types::Object>();
    if (!object) {
      continue;
    }
    auto relation = hosting.negotiate(requirement, *object);
    if (relation == Ttx::Concept::Interface::Relation::Rejected) {
      continue;
    }

    auto type_index = find_configured_type(configured, *object);
    BAIL_IF(!type_index);
    hosted.insert(Products::Hosted(*field, *type_index));
  }
  return Products(scene, hosted.get_view());
}
