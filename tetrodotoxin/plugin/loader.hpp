// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

#include "tetrodotoxin/plugin/abi.h"

namespace Tetrodotoxin::Plugin {

struct RequirementDescriptor {
  std::string package_coordinate;
  std::string exported_route;
  std::string contract_version;
  std::array<uint8_t, 32> content_sha256;
};

struct ProviderDescriptor {
  std::string package_coordinate;
  std::string exported_route;
  std::string export_version;
  std::array<uint8_t, 32> content_sha256;
  ttx_abstract candidate;
  tetrodotoxin_terminal_provider terminal;
  tetrodotoxin_dialect_provider dialect;
};

enum class LoadFailure {
  LibraryUnavailable,
  EntryUnavailable,
  EntryRejected,
  IncompatibleAbi,
  InvalidProtocol,
};

// Loaded keeps the native library mapped for as long as any exported Abstract
// can be observed. Provider descriptors are copied during synchronous visits,
// while the candidate handles continue borrowing their operation tables from
// this retained Plugin.
class Loaded {
 public:
  static auto open(
      const std::string& path,
      ttx_host_requirements host,
      LoadFailure& failure) -> std::optional<Loaded>;

  Loaded(const Loaded&) = delete;
  Loaded& operator=(const Loaded&) = delete;
  Loaded(Loaded&& source) noexcept;
  Loaded& operator=(Loaded&& source) = delete;
  ~Loaded();

  auto providers(std::vector<ProviderDescriptor>& output) const -> bool;
  auto dependencies(
      ttx_abstract candidate,
      std::vector<RequirementDescriptor>& output) const -> bool;

  // close first prevents new plugin work, then releases the retained Plugin
  // and unmaps its library. A provider-owned close Error leaves both retained;
  // unloading code while it may still own callbacks would be less recoverable
  // than leaking the rejected plugin until process exit.
  auto close() -> bool;

 private:
  Loaded(void* library, ttx_plugin plugin) : library(library), plugin(plugin) {}

  void* library = nullptr;
  ttx_plugin plugin = {};
};

}  // namespace Tetrodotoxin::Plugin
