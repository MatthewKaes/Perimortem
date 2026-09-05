// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/types/contiguous.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/concept/unknown.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Access is one writable contiguous storage Type. It retains the exact element
// edge while its Generic owns the canonical materialization key.
class Access : public Contiguous {
 public:

  constexpr Access(
      Perimortem::Core::View::Bytes name,
      const Model::Type& element)
      : name(name), element(element) {}

  Access(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Bytes name,
      const Model::Type& element,
      const Model::Type& size_type,
      const Model::Type& flag_type,
      const Model::Type& view_type);

  TTX_NAME(name);

  TTX_DOCUMENTATION(documentation);

  auto initialize_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  constexpr auto get_element_type() const -> const Model::Type& override {
    return element;
  }

 private:
  Perimortem::Core::View::Bytes name;
  const Model::Type& element;
  static constexpr Ttx::Documentations::Comment documentation{
    "Provides writable access to contiguous values."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
