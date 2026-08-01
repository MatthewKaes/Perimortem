// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Void is the empty Library result Type. Its inherited empty Type Layout is a
// complete semantic shape without inventing Value width, size, or alignment.
class Void : public Ttx::Model::Type {
 public:
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Void"_view;
  }

  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return documentation;
  }

  auto resolve_context(Perimortem::Core::View::Bytes) const
      -> const Ttx::Concept::Abstract& override {
    return Ttx::Concept::Invalid::get_invalid();
  }

 private:
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Void is the empty result Type for Library Callables."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
