// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/model/operations/binary.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Model::Shaders {

// Binary resolves the ordered Type arguments of one primitive Shader binary
// operation. It is an identity-free legality policy, not a Callable, Generic,
// registry, or durable semantic owner. The Body retains the selected left,
// right, and result Types through its ordinary Value table.
class Binary {
 public:
  static auto resolve(
      Tetrodotoxin::Model::Operations::Binary::Operator operation,
      const Ttx::Model::Type& left,
      const Ttx::Model::Type& right) -> const Ttx::Concept::Abstract&;
};

}  // namespace Tetrodotoxin::Model::Shaders
