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
  TTX_NAME("Void"_view);

  TTX_CONSTEXPR_DOCUMENTATION(documentation);

  TTX_INVALID_CONTEXT;

 private:
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Void is the empty result Type for Library Callables."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
