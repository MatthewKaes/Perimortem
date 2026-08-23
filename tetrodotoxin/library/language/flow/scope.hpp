// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/model/type.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/layout.hpp"

namespace Tetrodotoxin::Library::Language::Flow {

// Scope is the Library execution context available to one retained Statement.
// It exposes only the authority that expression and control owners cannot infer
// from their own graph edges. This keeps those queries out of host neutral TTX
// and prevents an embedded expression from requiring a concrete Block.
class Scope : public Ttx::Concept::Abstract {
 public:
  TTX_CONTRACT(Scope, Ttx::Concept::Abstract);

  virtual constexpr auto get_access_scope() const -> const Model::Type& = 0;

  virtual constexpr auto get_function_results() const
      -> const Ttx::Concept::Layout& = 0;

  virtual constexpr auto get_enclosing_loop() const
      -> Perimortem::Core::Option<const Ttx::Concept::Abstract&> = 0;
};

}  // namespace Tetrodotoxin::Library::Language::Flow
