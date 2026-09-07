// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <memory>
#include <vector>

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/dialect.hpp"

namespace Tetrodotoxin::Environment {

// Installation keeps provider code and names available to every borrowing
// Workspace. Native construction is a convenience at this boundary. Lookup and
// interpretation use the same retained provider handle for every language.
class Toolchain {
 public:
  Toolchain() = default;
  ~Toolchain();
  Toolchain(const Toolchain&) = delete;
  auto operator=(const Toolchain&) -> Toolchain& = delete;

  template <typename Target, typename... Dependencies>
  auto install(
      Perimortem::Core::View::Bytes name,
      Dependencies&... dependencies) -> Perimortem::Core::Option<Target&> {
    if (find(name).operations || !(contains(dependencies.get_abi()) && ...)) {
      return {};
    }
    const auto retained_name = arena.proxy(name);
    auto owner = std::make_unique<Target>(retained_name, dependencies...);
    auto& concrete = *owner;
    const bool installed = install(owner->get_provider());
    if (!installed) {
      return {};
    }
    native_owners.push_back(std::move(owner));
    return concrete;
  }
  auto install(tetrodotoxin_dialect_provider provider) -> bool;
  auto find(Perimortem::Core::View::Bytes name) const
      -> tetrodotoxin_dialect_provider;
  auto contains(ttx_abstract candidate) const -> bool;
  auto is_installed(tetrodotoxin_dialect_provider provider) const -> bool;

 private:
  Perimortem::Memory::Allocator::Arena arena;
  std::vector<std::unique_ptr<Language::Dialect>> native_owners;
  std::vector<tetrodotoxin_dialect_provider> providers;
};

}  // namespace Tetrodotoxin::Environment
