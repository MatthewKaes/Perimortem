// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "backend/llvm/compiler.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "backend/llvm/lowering/graph.hpp"
#include "backend/llvm/representation/program.hpp"
#include "llvm-c/Core.h"
#include "llvm/Config/llvm-config.h"

using namespace Perimortem;
using namespace Tetrodotoxin::Backend;

auto Llvm::Compiler::compile(
    Memory::Allocator::Arena& arena,
    const Request& request) const -> Utility::Result<Products, Failure> {
  U32 major = 0;
  U32 minor = 0;
  U32 patch = 0;
  LLVMGetVersion(&major, &minor, &patch);
  if (major != LLVM_VERSION_MAJOR || minor != LLVM_VERSION_MINOR ||
      patch != LLVM_VERSION_PATCH) {
    Core::Diagnostics::Log::error(
        "The loaded LLVM runtime does not match the pinned SDK."_view);
    return Failure::ToolchainFailed;
  }

  Llvm::Representation::Program program(
      arena, request.get_errors(), request.get_source_path(),
      request.get_source_text(), request.get_target(),
      request.get_debug_level(), request.get_unit());
  Bool initialized = program.initialize();
  if (!initialized) {
    return Failure::ToolchainFailed;
  }

  if (!Llvm::Lowering::Graph::lower(program, request.get_monograph())) {
    return program.has_source_failure() ? Failure::SourceRejected
                                        : Failure::ToolchainFailed;
  }

  return program.compile();
}
