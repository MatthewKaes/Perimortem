// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "backend/llvm/lowering/execution.hpp"

namespace Tetrodotoxin::Backend::Llvm::Lowering {

// Control follows Library Statements in authored order and creates the native
// branches and lifetime scopes required by the completed semantic flow.
class Control {
 public:
  static auto lower(
      const Execution& execution,
      const Tetrodotoxin::Library::Language::Statement& statement) -> Bool;

  static auto lower(
      const Execution& execution,
      const Tetrodotoxin::Library::Language::Flow::Block& block) -> Bool;
};

}  // namespace Tetrodotoxin::Backend::Llvm::Lowering
