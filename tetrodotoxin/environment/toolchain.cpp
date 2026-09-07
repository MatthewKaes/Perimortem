// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/environment/toolchain.hpp"

using namespace Tetrodotoxin::Environment;
using namespace Perimortem::Core;

Toolchain::~Toolchain() {
  for (auto it = providers.rbegin(); it != providers.rend(); ++it) {
    it->operations->release(it->self);
  }
  while (!native_owners.empty()) {
    native_owners.pop_back();
  }
}

auto Toolchain::install(tetrodotoxin_dialect_provider provider) -> bool {
  const auto* ops = provider.operations;
  if (!ops || !provider.self || ops->header.abi_major != TTX_ABI_MAJOR ||
      ops->header.size < sizeof(tetrodotoxin_dialect_provider_ops) ||
      !ops->candidate || !ops->name || !ops->retain || !ops->release ||
      !ops->interpret) {
    return false;
  }
  const auto candidate = ops->candidate(provider.self);
  const auto name = ops->name(provider.self);
  const auto proof =
      Ttx::relation(candidate, tetrodotoxin_dialect_requirement());
  if (!candidate || !name.data || !name.size ||
      find({name.data, name.size}).operations ||
      (proof != TTX_INTERFACE_SATISFIED && proof != TTX_INTERFACE_EQUIVALENT)) {
    return false;
  }
  ops->retain(provider.self);
  providers.push_back(provider);
  return true;
}

auto Toolchain::find(View::Bytes name) const -> tetrodotoxin_dialect_provider {
  for (auto provider : providers) {
    const auto candidate = provider.operations->name(provider.self);
    if (name == View::Bytes(candidate.data, candidate.size)) {
      return provider;
    }
  }
  return {};
}

auto Toolchain::contains(ttx_abstract candidate) const -> bool {
  for (auto provider : providers) {
    if (ttx_abstract_same(
            candidate, provider.operations->candidate(provider.self))) {
      return true;
    }
  }
  return false;
}

auto Toolchain::is_installed(tetrodotoxin_dialect_provider candidate) const
    -> bool {
  for (auto provider : providers) {
    if (candidate.operations == provider.operations &&
        candidate.self == provider.self) {
      return true;
    }
  }
  return false;
}
