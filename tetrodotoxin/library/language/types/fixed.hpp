// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/types/contiguous.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/layouts/ranged.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Fixed is one homogeneous range Type. It retains the original unsigned extent
// and element edge while Ranged exposes the repeated identity without copies.
class Fixed : public Contiguous {
 public:
  TTX_CONTRACT(Fixed, Contiguous, 0xf1d212690ed5471f, 0x8f47246bd80d2cbe);

  Fixed(
      Perimortem::Core::View::Bytes name,
      const Ttx::Model::Type& element,
      ::Unsigned_64 extent)
      : name(name),
        element(element),
        extent(extent),
        layout(element, Count(extent)) {}

  TTX_NAME(name);

  TTX_DOCUMENTATION(documentation);

  TTX_CONSTEXPR_INVALID_CONTEXT;

  constexpr auto get_layout() const
      -> const Ttx::Model::Layouts::Ranged& override {
    return layout;
  }

  constexpr auto get_element_type() const -> const Ttx::Model::Type& override {
    return element;
  }

  constexpr auto get_extent() const -> ::Unsigned_64 { return extent; }

 private:
  Perimortem::Core::View::Bytes name;
  const Ttx::Model::Type& element;
  ::Unsigned_64 extent;
  Ttx::Model::Layouts::Ranged layout;
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Creates a fixed homogeneous range Type."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
