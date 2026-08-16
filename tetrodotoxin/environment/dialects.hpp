// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/dialect.hpp"

namespace Tetrodotoxin::Environment {

// Owns the installed Dialect inventory for one Workspace. Its Arena stores only
// Workspace lifetime Dialect state. Each source graph uses an independent
// transaction Arena that Workspace may release without disturbing installation.
class Dialects {
 public:
  Dialects(Perimortem::Memory::Allocator::Arena& arena);
  ~Dialects();

  template <typename TargetDialect, typename... DependencyDialects>
  auto install(
      Perimortem::Core::View::Bytes name,
      DependencyDialects&... dependencies) -> TargetDialect* {
    if (contains_name(name)) {
      return nullptr;
    }

    Bool dependencies_installed =
        (contains_instance(static_cast<Language::Dialect&>(dependencies)) &&
         ...);
    if (!dependencies_installed) {
      return nullptr;
    }

    Perimortem::Core::View::Bytes retained_name = arena.proxy(name);
    auto& dialect =
        arena.construct<TargetDialect>(retained_name, dependencies...);
    instances.insert(&dialect);
    return &dialect;
  }

  auto get_dialects() const
      -> Perimortem::Core::View::Vector<Language::Dialect*>;

 private:
  auto contains_name(Perimortem::Core::View::Bytes name) const -> Bool;
  auto contains_instance(const Language::Dialect& dialect) const -> Bool;
  Perimortem::Memory::Allocator::Arena& arena;
  Perimortem::Memory::Managed::Vector<Language::Dialect*> instances;
};

}  // namespace Tetrodotoxin::Environment
