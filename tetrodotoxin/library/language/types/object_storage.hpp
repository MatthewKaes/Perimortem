// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/type.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/comment.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// ObjectStorage is the generated Object[T] value. It owns one carrier that can
// be empty managed buffer while its Generic retains the canonical element
// identity.
class ObjectStorage : public Model::Type {
 public:
  TTX_CONTRACT(ObjectStorage, Model::Type);

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
  TTX_CONSTEXPR_INVALID_CONTEXT;

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  constexpr auto get_element_type() const -> const Model::Type& {
    return element;
  }

 private:
  Perimortem::Core::View::Bytes name;
  const Model::Type& element;
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Owns an empty-capable managed buffer of one exact element Type."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
