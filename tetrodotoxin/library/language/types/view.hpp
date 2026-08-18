// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/types/contiguous.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/comment.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// View is one read only contiguous storage Type. It retains the exact element
// edge while its Generic owns the canonical materialization key.
class View : public Contiguous {
 public:
  TTX_CONTRACT(View, Contiguous);

  constexpr View(Perimortem::Core::View::Bytes name, const Model::Type& element)
      : name(name), element(element) {}

  View(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Bytes name,
      const Model::Type& element,
      const Model::Type& size_type);

  TTX_NAME(name);

  TTX_DOCUMENTATION(documentation);

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  auto reserve(Llvm::Program& program) const -> Bool override;

  auto complete(Llvm::Program& program) const -> Bool override;
  TTX_CONSTEXPR_INVALID_CONTEXT;

  constexpr auto get_element_type() const -> const Model::Type& override {
    return element;
  }

 private:
  Perimortem::Core::View::Bytes name;
  const Model::Type& element;
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Provides read-only access to contiguous values."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
