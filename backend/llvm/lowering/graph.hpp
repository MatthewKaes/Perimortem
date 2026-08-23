// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "backend/llvm/representation/program.hpp"
#include "tetrodotoxin/library/language/model/addressable.hpp"
#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"

namespace Tetrodotoxin::Backend::Llvm::Lowering {

// Graph is the LLVM entry into one completed Library Monograph. It walks the
// real semantic identities through reservation, target completion, and source
// ordered emission without attaching physical state to the Dialect.
class Graph {
 public:
  static auto lower(
      Representation::Program& program,
      const Tetrodotoxin::Library::Language::Monograph& monograph) -> Bool;

  static auto prepare(
      Representation::Program& program,
      const Tetrodotoxin::Library::Language::Model::Callable& callable) -> Bool;

  static auto prepare(
      Representation::Program& program,
      const Tetrodotoxin::Library::Language::Model::Addressable& addressable)
      -> Bool;
};

}  // namespace Tetrodotoxin::Backend::Llvm::Lowering
