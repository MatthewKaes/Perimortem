// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/environment/dialect.hpp"
#include "tetrodotoxin/language/dialect.hpp"

namespace Tetrodotoxin::Build {

// Build interprets one infrastructure root against the exact Environment
// Dialect installed beneath it. It records product requests and deferred
// provider locators; Environment later commits those locators to plugin-owned
// identities before constructing the Package Workspace.
class Dialect final : public Tetrodotoxin::Language::Dialect {
 public:
  Dialect(
      Perimortem::Core::View::Bytes name,
      Tetrodotoxin::Environment::Dialect& environment)
      : Tetrodotoxin::Language::Dialect(name), environment(environment) {}

  auto interpret(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation,
      const Ttx::Lexical::Anchor& source_anchor,
      Ttx::Concept::Abstract& context)
      -> Perimortem::Core::Option<Tetrodotoxin::Language::Monograph&> override;

  void produce(
      ttx_context context,
      Perimortem::Memory::Allocator::Arena& arena,
      tetrodotoxin_workspace_view workspace,
      const Tetrodotoxin::Language::Monograph& monograph,
      tetrodotoxin_production_result result) const override;

  constexpr auto get_environment() const
      -> Tetrodotoxin::Environment::Dialect& {
    return environment;
  }

 private:
  Tetrodotoxin::Environment::Dialect& environment;
};

}  // namespace Tetrodotoxin::Build
