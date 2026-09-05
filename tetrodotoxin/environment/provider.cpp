// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/environment/provider.h"

#include "ttx/model/requirement.hpp"

static auto workspace_requirement() -> ttx_abstract {
  static constinit Ttx::Requirement requirement(
      "Tetrodotoxin.Environment.Workspace"_bytes);
  return requirement.get_abi();
}

extern "C" ttx_abstract TTX_CALL tetrodotoxin_workspace_requirement(void) {
  return workspace_requirement();
}
