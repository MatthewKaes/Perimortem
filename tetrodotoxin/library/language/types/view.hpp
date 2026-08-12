// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// View is one read only contiguous storage Type. It retains the exact element
// edge while Materializations alone owns the Generic key that created it.
class View : public Ttx::Model::Type {
 public:
  TTX_CONTRACT(View, Ttx::Model::Type, 0xdbd463c86a2c4a08, 0xb42869ffad9e4fc9);

  constexpr View(
      Perimortem::Core::View::Bytes name,
      const Ttx::Model::Type& element)
      : name(name), element(element) {}

  TTX_NAME(name);

  TTX_DOCUMENTATION(documentation);

  TTX_CONSTEXPR_INVALID_CONTEXT;

  constexpr auto get_element_type() const -> const Ttx::Model::Type& {
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
