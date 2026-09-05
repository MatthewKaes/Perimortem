// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/initialization.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/concept/unknown.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// ObjectStorage is the generated Object[T] value. It owns one carrier that can
// be empty managed buffer while its Generic retains the canonical element
// identity.
class ObjectStorage : public Model::Type {
 public:

  ObjectStorage(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Bytes name,
      const Model::Type& element,
      const Model::Type& size_type,
      const Model::Type& flag_type,
      const Model::Type& view_type,
      const Model::Type& access_type);

  TTX_NAME(name);
  TTX_DOCUMENTATION(documentation);

  auto initialize_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&>;

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;
  void visit_concepts(ttx_named_abstract_callable* visitor) const override;

  constexpr auto get_element_type() const -> const Model::Type& {
    return element;
  }

 private:
  Perimortem::Core::View::Bytes name;
  const Model::Type& element;
  Model::OwnedInitialization<ObjectStorage> initialization;
  static constexpr Ttx::Documentations::Comment documentation{
    "Owns an empty-capable managed buffer of one exact element Type."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
