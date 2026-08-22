// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Library::Llvm {

// SourceRejected means the backend published an actionable source report.
// ToolchainFailed means the process diagnostic log owns the failure detail.
enum class Failure : U8 {
  SourceRejected,
  ToolchainFailed,
};

}  // namespace Tetrodotoxin::Library::Llvm
