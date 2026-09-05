// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/provider.h"

#include "ttx/model/requirement.hpp"

static auto dialect_requirement() -> ttx_abstract {
  static constinit Ttx::Requirement requirement(
      "Tetrodotoxin.Language.Dialect"_bytes);
  return requirement.get_abi();
}

extern "C" TTX_EXPORT ttx_abstract TTX_CALL
    tetrodotoxin_dialect_requirement(void) {
  return dialect_requirement();
}
