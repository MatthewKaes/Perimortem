// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/llvm/compiler.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "llvm-c/Core.h"
#include "llvm/Config/llvm-config.h"
#include "tetrodotoxin/library/llvm/program.hpp"

using namespace Perimortem;

auto Tetrodotoxin::Library::Llvm::Compiler::compile(
    Memory::Allocator::Arena& arena,
    const Request& request) const -> Utility::Result<Products, Failure> {
  Unsigned_32 major = 0;
  Unsigned_32 minor = 0;
  Unsigned_32 patch = 0;
  LLVMGetVersion(&major, &minor, &patch);
  if (major != LLVM_VERSION_MAJOR || minor != LLVM_VERSION_MINOR ||
      patch != LLVM_VERSION_PATCH) {
    Core::Diagnostics::Log::error(
        "The loaded LLVM runtime does not match the pinned SDK."_view);
    return Failure::ToolchainFailed;
  }

  Program program(
      arena, request.get_errors(), request.get_source_path(),
      request.get_source_text(), request.get_target(),
      request.get_debug_level(), request.get_unit());
  Bool initialized = program.initialize();
  if (!initialized) {
    return Failure::ToolchainFailed;
  }

  auto lowered = request.get_monograph().lower(program);
  if (!lowered) {
    return program.has_source_failure() ? Failure::SourceRejected
                                        : Failure::ToolchainFailed;
  }

  return program.compile();
}
