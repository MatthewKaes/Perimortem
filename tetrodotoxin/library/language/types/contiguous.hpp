// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Contiguous is the shared Type contract for indexed value storage. The
// receiver remains one value of its exact concrete Type while this category
// exposes the exact element Type consumed by Slice. Access retains the writable
// subset used by Index.
class Contiguous : public Ttx::Model::Type {
 public:
  TTX_CONTRACT(Contiguous, Ttx::Model::Type);

  virtual constexpr auto get_element_type() const
      -> const Ttx::Model::Type& = 0;
};

}  // namespace Tetrodotoxin::Library::Language::Types
