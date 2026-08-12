// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Void is the empty Library result Type. Its explicit empty Layout is a
// complete zero value shape without inventing scalar storage or identity.
class Void : public Ttx::Model::Type {
 public:
  TTX_NAME("Void"_view);

  TTX_DOCUMENTATION(documentation);

  TTX_INVALID_CONTEXT;

  constexpr auto get_layout() const
      -> const Ttx::Model::Layouts::Fluid& override {
    return empty_layout;
  }

 private:
  static constexpr Ttx::Model::Layouts::Fluid empty_layout;
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Void is the empty result Type for Library Callables."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
