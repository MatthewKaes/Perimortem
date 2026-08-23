// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"

namespace Tetrodotoxin::Language {

// The consumer constructs Report because only it has the authored source name,
// body, and range. A concrete Error contributes its retained owner facts to
// that Report without moving source or publication state into this cross
// Dialect contract.
class Error : public Ttx::Concept::Abstract {
 public:
  TTX_CONTRACT(Error, Ttx::Concept::Abstract);

  TTX_NAME("Error"_view);

  TTX_EMPTY_DOCUMENTATION();

  TTX_CONSTEXPR_INVALID_CONTEXT;

  virtual auto describe(Ttx::Lexical::Errors::Report& report) const -> void = 0;
};

}  // namespace Tetrodotoxin::Language
