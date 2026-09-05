// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/model/admission.hpp"
#include "tetrodotoxin/library/language/types/contiguous.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/concept/unknown.hpp"
#include "ttx/reference/model/layouts/ranged.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Fixed is one homogeneous range Type. It retains the original unsigned extent
// and element edge while Ranged exposes the repeated identity without copies.
class Fixed : public Contiguous {
 public:

  Fixed(
      Perimortem::Core::View::Bytes name,
      const Model::Type& element,
      ::U64 extent)
      : name(name),
        element(element),
        extent(extent),
        layout(element, Count(extent)),
        admission(*this) {}

  Fixed(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Bytes name,
      const Model::Type& element,
      ::U64 extent,
      const Model::Type& access_type,
      const Model::Type& view_type);

  TTX_NAME(name);

  TTX_DOCUMENTATION(documentation);

  auto initialize_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  auto accepts(const Model::Pack& source) const -> Bool;

  auto create_admitted(
      Perimortem::Memory::Allocator::Arena& arena,
      Model::Pack& source) const -> Perimortem::Core::Option<Model::Pack&>;

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;
  void visit_concepts(ttx_named_abstract_callable* visitor) const override;

  constexpr auto get_layout() const
      -> const Ttx::Model::Layouts::Ranged& override {
    return layout;
  }

  constexpr auto get_element_type() const -> const Model::Type& override {
    return element;
  }

  constexpr auto get_extent() const -> ::U64 { return extent; }

 private:
  Perimortem::Core::View::Bytes name;
  const Model::Type& element;
  ::U64 extent;
  Ttx::Model::Layouts::Ranged layout;
  Model::OwnedAdmission<Fixed> admission;
  static constexpr Ttx::Documentations::Comment documentation{
    "Creates a fixed homogeneous range Type."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
