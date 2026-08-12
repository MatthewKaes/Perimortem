// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Access is one writable contiguous storage Type. It retains the exact element
// edge while Materializations alone owns the Generic key that created it.
class Access : public Ttx::Model::Type {
 public:
  TTX_CONTRACT(
      Access,
      Ttx::Model::Type,
      0x9297f2706d2e464e,
      0x82d4b4fece93716d);

  constexpr Access(
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
    "Provides writable access to contiguous values."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
