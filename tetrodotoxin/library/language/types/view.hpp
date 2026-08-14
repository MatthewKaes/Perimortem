// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/types/contiguous.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/comment.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// View is one read only contiguous storage Type. It retains the exact element
// edge while Materializations alone owns the Generic key that created it.
class View : public Contiguous {
 public:
  TTX_CONTRACT(View, Contiguous);

  constexpr View(
      Perimortem::Core::View::Bytes name,
      const Ttx::Model::Type& element)
      : name(name), element(element) {}

  TTX_NAME(name);

  TTX_DOCUMENTATION(documentation);

  TTX_CONSTEXPR_INVALID_CONTEXT;

  constexpr auto get_element_type() const -> const Ttx::Model::Type& override {
    return element;
  }

 private:
  Perimortem::Core::View::Bytes name;
  const Ttx::Model::Type& element;
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Provides read-only access to contiguous values."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
