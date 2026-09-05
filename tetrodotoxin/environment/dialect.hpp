// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/dialect.hpp"
#include "tetrodotoxin/language/production.h"

namespace Tetrodotoxin::Environment {

struct ReachableRoot {
  enum class Kind : U8 {
    Project,
    Sdk,
  };

  Kind kind;
  Perimortem::Core::View::Bytes locator;
};

struct ProductDescription {
  Perimortem::Core::View::Bytes name;
  Perimortem::Core::View::Bytes package_route;
  Perimortem::Core::View::Bytes terminal_artifact;
  Perimortem::Core::View::Bytes terminal_export;
};

class Plan {
 public:
  Plan(const Plan&) = delete;
  Plan(Plan&&) = delete;
  auto operator=(const Plan&) -> Plan& = delete;
  auto operator=(Plan&&) -> Plan& = delete;

 private:
  friend class Dialect;

  Plan(
      Perimortem::Core::View::Bytes output_root,
      Perimortem::Memory::Managed::Vector<ReachableRoot> reachable_roots,
      Perimortem::Memory::Managed::Vector<Perimortem::Core::View::Bytes>
          repositories,
      Perimortem::Memory::Managed::Vector<Perimortem::Core::View::Bytes>
          plugins)
      : output_root(output_root),
        reachable_roots(reachable_roots),
        repositories(repositories),
        plugins(plugins) {}

  Perimortem::Core::View::Bytes output_root;
  Perimortem::Memory::Managed::Vector<ReachableRoot> reachable_roots;
  Perimortem::Memory::Managed::Vector<Perimortem::Core::View::Bytes>
      repositories;
  Perimortem::Memory::Managed::Vector<Perimortem::Core::View::Bytes> plugins;
};

// Environment is installed as a bootstrap dependency of Build. Build delegates
// its inline host description to this exact Dialect, while Environment keeps
// plugin lifetime, reachable roots, the child Toolchain, and the child
// Workspace behind one opaque relationship. Selecting Environment as a root
// source is rejected because it has no independent semantic product.
class Dialect final : public Tetrodotoxin::Language::Dialect {
 public:
  explicit Dialect(Perimortem::Core::View::Bytes name)
      : Tetrodotoxin::Language::Dialect(name) {}

  auto interpret(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation,
      const Ttx::Lexical::Anchor& source_anchor,
      Ttx::Concept::Abstract& context)
      -> Perimortem::Core::Option<Tetrodotoxin::Language::Monograph&> override;

  // Build lends the inline Environment region and its source transaction to
  // this exact installed Dialect. Parsing the region here keeps host locators
  // and provider admission syntax out of Build while retaining every authored
  // choice in the source Arena for the child Environment to realize later.
  auto parse_region(Ttx::Lexical::Cursor& cursor) const
      -> Perimortem::Core::Option<Plan&>;

  // Build lends its authored locators back to the Environment that parsed the
  // inline region. Environment resolves those locators, constructs one child
  // toolchain and Workspace, and returns only the resulting publication Pack;
  // Build never receives the provider, repository, or host-toolchain objects.
  void produce_build(
      const Plan& plan,
      Perimortem::Core::View::Bytes build_source,
      Perimortem::Core::View::Bytes package_locator,
      Perimortem::Core::View::Vector<ProductDescription> products,
      const Ttx::Concept::Abstract& graph,
      ttx_context context,
      Perimortem::Memory::Allocator::Arena& result_arena,
      tetrodotoxin_production_result result) const;
};

}  // namespace Tetrodotoxin::Environment
