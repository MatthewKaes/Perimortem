// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Descriptor is the Library value Type produced when an Expression selects a
// semantic Type identity. The selected identity remains the Expression result;
// Descriptor only prevents that compile-time value from masquerading as an
// instance of the selected Type in ordinary value operations.
class Descriptor : public Ttx::Model::Type {
 public:
  TTX_NAME("<descriptor>"_view);

  TTX_DOCUMENTATION(documentation);

  TTX_INVALID_CONTEXT;

 private:
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Descriptor is the Library value Type for a selected semantic Type."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
