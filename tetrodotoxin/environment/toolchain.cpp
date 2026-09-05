// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/environment/toolchain.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;

Environment::Toolchain::Toolchain() : dialects(arena) {}

Environment::Toolchain::~Toolchain() {
  for (Count index = dialects.get_size(); index > 0; index--) {
    const Language::InstalledDialect& installed = dialects[index - 1];
    if (installed.get_provider().operations != nullptr) {
      const tetrodotoxin_dialect_provider provider = installed.get_provider();
      provider.operations->release(provider.self);
    } else if (installed.get_local() != nullptr) {
      installed.get_local()->~Dialect();
    }
  }
}

auto Environment::Toolchain::contains_name(View::Bytes name) const -> Bool {
  return dialects.get_view().contains(
      [&](const auto& dialect) { return dialect.get_name() == name; });
}

auto Environment::Toolchain::contains(const Language::Dialect& dialect) const
    -> Bool {
  return dialects.get_view().contains(
      [&](const auto& candidate) { return candidate.get_local() == &dialect; });
}

auto Environment::Toolchain::install(tetrodotoxin_dialect_provider provider)
    -> Bool {
  BAIL_IF(
      provider.operations == nullptr || provider.self == nullptr ||
      provider.operations->header.abi_major != TTX_ABI_MAJOR ||
      provider.operations->header.size <
          sizeof(tetrodotoxin_dialect_provider_ops) ||
      provider.operations->retain == nullptr ||
      provider.operations->release == nullptr ||
      provider.operations->candidate == nullptr ||
      provider.operations->name == nullptr ||
      provider.operations->interpret == nullptr);

  const ttx_abstract candidate = provider.operations->candidate(provider.self);
  const ttx_borrowed_bytes supplied_name =
      provider.operations->name(provider.self);
  BAIL_IF(
      candidate.operations == nullptr ||
      Ttx::relation(candidate, tetrodotoxin_dialect_requirement()) !=
          TTX_INTERFACE_SATISFIED ||
      supplied_name.size == 0 || supplied_name.data == nullptr);
  View::Bytes name(supplied_name.data, Count(supplied_name.size));
  BAIL_IF(contains_name(name));

  provider.operations->retain(provider.self);
  View::Bytes retained_name = arena.proxy(name);
  dialects.insert(
      Language::InstalledDialect(retained_name, candidate, provider, nullptr));
  return True;
}

auto Environment::Toolchain::install(
    tetrodotoxin_dialect_provider provider,
    Language::Dialect& local) -> Bool {
  BAIL_IF(
      provider.operations == nullptr ||
      !ttx_abstract_same(
          provider.operations->candidate(provider.self), local.get_handle()));
  BAIL_IF(!install(provider));
  Language::InstalledDialect& installed = dialects[dialects.get_size() - 1];
  installed = Language::InstalledDialect(
      installed.get_name(), installed.get_candidate(), installed.get_provider(),
      &local);
  return True;
}

auto Environment::Toolchain::find(View::Bytes name) const
    -> Option<Language::Dialect&> {
  for (const Language::InstalledDialect& installed : dialects.get_view()) {
    if (installed.get_name() == name && installed.get_local() != nullptr) {
      return *installed.get_local();
    }
  }

  return {};
}

auto Environment::Toolchain::find_provider(View::Bytes name) const
    -> Option<tetrodotoxin_dialect_provider> {
  for (const Language::InstalledDialect& installed : dialects.get_view()) {
    if (installed.get_name() == name &&
        installed.get_provider().operations != nullptr) {
      return installed.get_provider();
    }
  }
  return {};
}

auto Environment::Toolchain::is_installed(
    tetrodotoxin_dialect_provider provider) const -> Bool {
  return dialects.get_view().contains(
      [&](const Language::InstalledDialect& installed) {
        const tetrodotoxin_dialect_provider candidate =
            installed.get_provider();
        return candidate.operations == provider.operations &&
               candidate.self == provider.self;
      });
}
