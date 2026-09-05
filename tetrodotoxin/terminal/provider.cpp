// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/provider.h"

#include "ttx/model/requirement.hpp"

static auto terminal_requirement() -> ttx_abstract {
  static constinit Ttx::Requirement requirement(
      "Tetrodotoxin.Terminal.Provider"_bytes);
  return requirement.get_abi();
}

extern "C" TTX_EXPORT ttx_abstract TTX_CALL
    tetrodotoxin_terminal_requirement(void) {
  return terminal_requirement();
}
