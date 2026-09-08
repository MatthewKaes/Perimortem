// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once
#include "perimortem/memory/managed/vector.hpp"

namespace Tetrodotoxin::Environment {
// Plan retains the host choices made by a Build source. The invocation resolves
// its locators before opening the product's child Workspace.
class Plan {
 public:
  struct ReachableRoot {
    enum class Kind : U8 { Project, Sdk };
    Kind kind;
    Perimortem::Core::View::Bytes locator;
  };
  Plan(
      Perimortem::Core::View::Bytes output_root,
      Perimortem::Memory::Managed::Vector<ReachableRoot> roots,
      Perimortem::Memory::Managed::Vector<Perimortem::Core::View::Bytes>
          repositories,
      Perimortem::Memory::Managed::Vector<Perimortem::Core::View::Bytes>
          plugins)
      : output_root(output_root),
        roots(roots),
        repositories(repositories),
        plugins(plugins) {}
  auto get_output_root() const -> Perimortem::Core::View::Bytes {
    return output_root;
  }
  auto get_roots() const -> Perimortem::Core::View::Vector<ReachableRoot> {
    return roots;
  }
  auto get_repositories() const
      -> Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> {
    return repositories;
  }
  auto get_plugins() const
      -> Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> {
    return plugins;
  }

 private:
  Perimortem::Core::View::Bytes output_root;
  Perimortem::Memory::Managed::Vector<ReachableRoot> roots;
  Perimortem::Memory::Managed::Vector<Perimortem::Core::View::Bytes>
      repositories;
  Perimortem::Memory::Managed::Vector<Perimortem::Core::View::Bytes> plugins;
};
}  // namespace Tetrodotoxin::Environment
