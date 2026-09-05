// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <optional>
#include <string>
#include <vector>

#include "tetrodotoxin/language/error.hpp"
#include "tetrodotoxin/plugin/loader.hpp"

namespace Tetrodotoxin::Environment {

struct InstalledRequirement {
  Plugin::RequirementDescriptor descriptor;
  ttx_abstract identity;
};

enum class ProviderAdmissionFailure {
  LibraryUnavailable,
  EntryUnavailable,
  EntryRejected,
  IncompatibleAbi,
  InvalidProtocol,
  UndeclaredRequirement,
  DuplicateExport,
};

// A loaded library lends operation tables and provider identities to its child
// toolchain. ProviderClosure retains that library and resolves every durable
// dependency descriptor against requirements installed by the surrounding SDK.
// Descriptors never manufacture graph identity: a match returns the exact
// Abstract supplied at construction, while a mismatch rejects the library
// before any of its candidates can enter a Workspace.
class ProviderClosure {
 public:
  explicit ProviderClosure(std::vector<InstalledRequirement> requirements);
  ~ProviderClosure();

  ProviderClosure(const ProviderClosure&) = delete;
  ProviderClosure(ProviderClosure&&) = delete;
  auto operator=(const ProviderClosure&) -> ProviderClosure& = delete;
  auto operator=(ProviderClosure&&) -> ProviderClosure& = delete;

  auto admit(const std::string& path, ProviderAdmissionFailure& failure)
      -> bool;

  // Selection is scoped to one explicitly admitted artifact and one complete
  // export descriptor. Environment can therefore commit an authored locator
  // without turning export names into a process-wide provider registry.
  auto select(
      const std::string& path,
      const Plugin::ProviderDescriptor& descriptor) const
      -> std::optional<ttx_abstract>;

  auto select_terminal(
      const std::string& path,
      const Plugin::ProviderDescriptor& descriptor) const
      -> std::optional<tetrodotoxin_terminal_provider>;

  auto find_terminal(const std::string& path, const std::string& exported_route)
      const -> std::optional<tetrodotoxin_terminal_provider>;

  auto select_dialect(
      const std::string& path,
      const Plugin::ProviderDescriptor& descriptor) const
      -> std::optional<tetrodotoxin_dialect_provider>;

  auto find_dialect(const std::string& path, const std::string& exported_route)
      const -> std::optional<tetrodotoxin_dialect_provider>;

  auto providers(const std::string& path) const
      -> std::vector<Plugin::ProviderDescriptor>;

 private:
  class RequirementError final : public Tetrodotoxin::Language::Error {
   public:
    TTX_NAME("Environment requirement error"_view);
    void describe(Ttx::Lexical::Errors::Report& report) const override;
  };

  struct LoadedProvider {
    std::string path;
    Plugin::Loaded plugin;
    std::vector<Plugin::ProviderDescriptor> providers;

    LoadedProvider(
        std::string path,
        Plugin::Loaded plugin,
        std::vector<Plugin::ProviderDescriptor> providers)
        : path(std::move(path)),
          plugin(std::move(plugin)),
          providers(std::move(providers)) {}

    LoadedProvider(const LoadedProvider&) = delete;
    LoadedProvider(LoadedProvider&&) noexcept = default;
    auto operator=(const LoadedProvider&) -> LoadedProvider& = delete;
    auto operator=(LoadedProvider&&) -> LoadedProvider& = delete;
  };

  struct HostBinding {
    ttx_host_requirements_ops operations;
    ProviderClosure* owner;
  };

  static void TTX_CALL resolve_requirement(
      ttx_host_requirements_self* self,
      const ttx_requirement_descriptor* requested,
      ttx_requirement_result result);

  auto find(const Plugin::RequirementDescriptor& requested) const
      -> const InstalledRequirement*;
  auto host() -> ttx_host_requirements;

  std::vector<InstalledRequirement> requirements;
  std::vector<LoadedProvider> loaded;
  HostBinding binding;
  RequirementError requirement_error;
};

}  // namespace Tetrodotoxin::Environment
