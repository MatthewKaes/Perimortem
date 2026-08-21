// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/dialect.hpp"
#include "ttx/concept/reference.hpp"

namespace Tetrodotoxin::Environment {

// Toolchain owns one immutable installed Dialect graph shared by every
// Workspace created by the host. Dialects retain only their configured names
// and downward dependency edges; semantic identities remain Workspace owned.
class Toolchain {
 public:
  Toolchain();
  ~Toolchain();

  Toolchain(const Toolchain&) = delete;
  Toolchain(Toolchain&&) = delete;
  auto operator=(const Toolchain&) -> Toolchain& = delete;
  auto operator=(Toolchain&&) -> Toolchain& = delete;

  template <typename TargetDialect, typename... DependencyDialects>
  auto install(
      Perimortem::Core::View::Bytes name,
      DependencyDialects&... dependencies)
      -> Perimortem::Core::Option<TargetDialect&> {
    BAIL_IF(contains_name(name));

    Bool dependencies_installed =
        (contains(static_cast<Language::Dialect&>(dependencies)) && ...);
    BAIL_IF(!dependencies_installed);

    Perimortem::Core::View::Bytes retained_name = arena.proxy(name);
    auto& dialect =
        arena.construct<TargetDialect>(retained_name, dependencies...);
    dialects.insert(dialect);
    return dialect;
  }

  auto find(Perimortem::Core::View::Bytes name) const
      -> Perimortem::Core::Option<Language::Dialect&>;

  constexpr auto get_dialects() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<Language::Dialect>> {
    return dialects;
  }

 private:
  auto contains_name(Perimortem::Core::View::Bytes name) const -> Bool;
  auto contains(const Language::Dialect& dialect) const -> Bool;

  Perimortem::Memory::Allocator::Arena arena;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Language::Dialect>>
      dialects;
};

}  // namespace Tetrodotoxin::Environment
