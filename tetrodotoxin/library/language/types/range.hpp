// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Range is one lazy integer sequence Type. It retains the exact element edge
// without claiming contiguous storage, state, or ownership of its Generic key.
class Range : public Ttx::Model::Type {
 public:
  TTX_CONTRACT(Range, Ttx::Model::Type);

  constexpr Range(
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
    "Provides a lazy ascending integer sequence."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
