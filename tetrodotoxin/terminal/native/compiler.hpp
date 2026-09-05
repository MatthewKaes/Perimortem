// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/utility/result.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "tetrodotoxin/terminal/native/toolchain.hpp"
#include "ttx/lexical/errors.hpp"

namespace Tetrodotoxin::Terminal::Native {

enum class Failure {
  InvalidProduct,
  IncompleteGraph,
  InterfaceFailed,
  CpuFailed,
  GpuFailed,
  ApplicationFailed,
  LinkerFailed,
};

// Compiler realizes one completed App from the live Package Workspace. Each
// semantic member stays with its original Dialect while independent Terminals
// project CPU objects, SPIR-V modules, embedded resources, and the application
// entry. Only the final executable leaves this owner.
class Compiler {
 public:
  auto compile(
      Perimortem::Memory::Allocator::Arena& arena,
      const Tetrodotoxin::Environment::Workspace& workspace,
      const Tetrodotoxin::Package::Language::Monograph& package,
      const Ttx::Concept::Abstract& product,
      const Tetrodotoxin::Library::Dialect& library,
      const Toolchain& toolchain,
      Ttx::Lexical::Errors& errors) const -> Perimortem::Utility::
      Result<Perimortem::Memory::Dynamic::Bytes, Failure>;
};

}  // namespace Tetrodotoxin::Terminal::Native
