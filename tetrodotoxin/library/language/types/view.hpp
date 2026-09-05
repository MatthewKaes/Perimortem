// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/admission.hpp"
#include "tetrodotoxin/library/language/types/contiguous.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/concept/unknown.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// View is one read only contiguous storage Type. It retains the exact element
// edge while its Generic owns the canonical materialization key.
class View : public Contiguous {
 public:

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;
  void visit_concepts(ttx_named_abstract_callable* visitor) const override;

  constexpr View(Perimortem::Core::View::Bytes name, const Model::Type& element)
      : name(name), element(element), admission(*this) {}

  View(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Bytes name,
      const Model::Type& element,
      const Model::Type& size_type,
      const Model::Type& flag_type);

  TTX_NAME(name);

  TTX_DOCUMENTATION(documentation);

  auto initialize_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  auto accepts(const Model::Pack& source) const -> Bool;

  constexpr auto get_element_type() const -> const Model::Type& override {
    return element;
  }

 private:
  Perimortem::Core::View::Bytes name;
  const Model::Type& element;
  Model::OwnedAdmission<View> admission;
  static constexpr Ttx::Documentations::Comment documentation{
    "Provides read-only access to contiguous values."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
