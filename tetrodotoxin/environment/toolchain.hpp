// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/dialect.hpp"

namespace Tetrodotoxin::Environment {

// Toolchain owns the installed family of Tetrodotoxin languages. Every
// Workspace can borrow the same immutable Dialect identities, while the
// semantic objects produced from a source stay with that Workspace. Installing
// dependencies first makes the language family one clear directed graph.
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
    const ttx_abstract candidate = dialect.get_handle();
    BAIL_IF(
        Ttx::relation(candidate, tetrodotoxin_dialect_requirement()) !=
        TTX_INTERFACE_SATISFIED);
    dialects.insert(
        Language::InstalledDialect(retained_name, candidate, {}, &dialect));
    return dialect;
  }

  // A foreign provider enters the same installed family as a native Dialect.
  // Toolchain retains its operation handle only after the candidate proves the
  // shared contract and its authored name is unique. No native object is
  // manufactured to imitate the provider.
  auto install(tetrodotoxin_dialect_provider provider) -> Bool;

  // A bundled C++ plugin may lend the exact Dialect behind its portable handle
  // after both the Dialect and SDK-owner relationships have been proved. The
  // provider retains that object; Toolchain releases the handle rather than
  // destroying memory owned by the loaded library.
  auto install(tetrodotoxin_dialect_provider provider, Language::Dialect& local)
      -> Bool;

  auto find(Perimortem::Core::View::Bytes name) const
      -> Perimortem::Core::Option<Language::Dialect&>;

  constexpr auto get_dialects() const
      -> Perimortem::Core::View::Vector<Language::InstalledDialect> {
    return dialects;
  }

  auto find_provider(Perimortem::Core::View::Bytes name) const
      -> Perimortem::Core::Option<tetrodotoxin_dialect_provider>;

  auto is_installed(tetrodotoxin_dialect_provider provider) const -> Bool;

 private:
  auto contains_name(Perimortem::Core::View::Bytes name) const -> Bool;
  auto contains(const Language::Dialect& dialect) const -> Bool;

  Perimortem::Memory::Allocator::Arena arena;
  Perimortem::Memory::Managed::Vector<Language::InstalledDialect> dialects;
};

}  // namespace Tetrodotoxin::Environment
