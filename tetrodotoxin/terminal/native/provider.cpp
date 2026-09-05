// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/native/provider.h"

#include "ttx/model/requirement.hpp"

static auto native_toolchain_requirement() -> ttx_abstract {
  static constinit Ttx::Requirement requirement(
      "Tetrodotoxin.Terminal.Native.Toolchain"_bytes);
  return requirement.get_abi();
}

extern "C" ttx_abstract TTX_CALL
    tetrodotoxin_native_toolchain_requirement(void) {
  return native_toolchain_requirement();
}
