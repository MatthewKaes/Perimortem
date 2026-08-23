// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Backend::Llvm {

// SourceRejected means the backend published an actionable source report.
// ToolchainFailed means the process diagnostic log owns the failure detail.
enum class Failure : U8 {
  SourceRejected,
  ToolchainFailed,
};

}  // namespace Tetrodotoxin::Backend::Llvm
