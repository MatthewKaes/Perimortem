// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/environment/provider_closure.hpp"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <limits>
#include <utility>

using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Environment;

void ProviderClosure::RequirementError::describe(
    Ttx::Lexical::Errors::Report& report) const {
  report
      << "Environment contains more than one owner for an exact plugin requirement."_view;
}

template <typename Operations>
static auto supports(const Operations* operations, uint32_t size) -> bool {
  return operations != nullptr &&
         operations->header.abi_major == TTX_PLUGIN_ABI_MAJOR &&
         operations->header.size >= size;
}

static auto bytes_equal(
    const std::string& installed,
    ttx_borrowed_bytes requested) -> bool {
  return requested.size <= std::numeric_limits<size_t>::max() &&
         requested.size == installed.size() &&
         (requested.size == 0 ||
          (requested.data != nullptr &&
           std::memcmp(requested.data, installed.data(), installed.size()) ==
               0));
}

static auto descriptor_equal(
    const Plugin::RequirementDescriptor& installed,
    const ttx_requirement_descriptor& requested) -> bool {
  return bytes_equal(
             installed.package_coordinate, requested.package_coordinate) &&
         bytes_equal(installed.exported_route, requested.exported_route) &&
         bytes_equal(installed.contract_version, requested.contract_version) &&
         std::equal(
             installed.content_sha256.begin(), installed.content_sha256.end(),
             requested.content_sha256);
}

static auto descriptor_equal(
    const Plugin::RequirementDescriptor& left,
    const Plugin::RequirementDescriptor& right) -> bool {
  return left.package_coordinate == right.package_coordinate &&
         left.exported_route == right.exported_route &&
         left.contract_version == right.contract_version &&
         left.content_sha256 == right.content_sha256;
}

static auto descriptor_equal(
    const Plugin::ProviderDescriptor& left,
    const Plugin::ProviderDescriptor& right) -> bool {
  return left.package_coordinate == right.package_coordinate &&
         left.exported_route == right.exported_route &&
         left.export_version == right.export_version &&
         left.content_sha256 == right.content_sha256;
}

ProviderClosure::ProviderClosure(std::vector<InstalledRequirement> requirements)
    : requirements(std::move(requirements)),
      loaded(),
      binding({
        .operations =
            {
              .header =
                  {
                    .size = sizeof(ttx_host_requirements_ops),
                    .abi_major = TTX_PLUGIN_ABI_MAJOR,
                    .abi_minor = TTX_PLUGIN_ABI_MINOR,
                  },
              .resolve = resolve_requirement,
            },
        .owner = this,
      }) {}

ProviderClosure::~ProviderClosure() {
  // Later plugins may borrow providers admitted earlier in the authored
  // closure. Closing in reverse order mirrors that dependency direction and
  // keeps every borrowed operation table mapped until its final dependent has
  // stopped calling it.
  while (!loaded.empty()) {
    // Loaded keeps a library mapped when its provider rejects close. Removing
    // the Environment record then ends host bookkeeping without unmapping
    // callbacks that may still belong to that provider.
    loaded.back().plugin.close();
    loaded.pop_back();
  }
}

auto ProviderClosure::host() -> ttx_host_requirements {
  return {
    .operations = &binding.operations,
    .self = reinterpret_cast<ttx_host_requirements_self*>(&binding),
  };
}

auto ProviderClosure::find(const Plugin::RequirementDescriptor& requested) const
    -> const InstalledRequirement* {
  for (const InstalledRequirement& installed : requirements) {
    if (descriptor_equal(installed.descriptor, requested)) {
      return &installed;
    }
  }
  return nullptr;
}

void ProviderClosure::resolve_requirement(
    ttx_host_requirements_self* self,
    const ttx_requirement_descriptor* requested,
    ttx_requirement_result result) {
  if (!supports(result.operations, sizeof(ttx_requirement_result_ops)) ||
      requested == nullptr ||
      requested->header.abi_major != TTX_PLUGIN_ABI_MAJOR ||
      requested->header.size < sizeof(ttx_requirement_descriptor)) {
    return;
  }
  auto& host = *reinterpret_cast<HostBinding*>(self);
  ProviderClosure& closure = *host.owner;
  const InstalledRequirement* installed = nullptr;
  for (const InstalledRequirement& candidate : closure.requirements) {
    if (descriptor_equal(candidate.descriptor, *requested)) {
      if (installed != nullptr) {
        result.operations->failed(
            result.self, closure.requirement_error.get_abi());
        return;
      }
      installed = &candidate;
    }
  }
  if (installed == nullptr) {
    result.operations->missing(result.self);
    return;
  }
  result.operations->resolved(result.self, installed->identity);
}

static auto translate(Plugin::LoadFailure failure) -> ProviderAdmissionFailure {
  switch (failure) {
  case Plugin::LoadFailure::LibraryUnavailable:
    return ProviderAdmissionFailure::LibraryUnavailable;
  case Plugin::LoadFailure::EntryUnavailable:
    return ProviderAdmissionFailure::EntryUnavailable;
  case Plugin::LoadFailure::EntryRejected:
    return ProviderAdmissionFailure::EntryRejected;
  case Plugin::LoadFailure::IncompatibleAbi:
    return ProviderAdmissionFailure::IncompatibleAbi;
  case Plugin::LoadFailure::InvalidProtocol:
    return ProviderAdmissionFailure::InvalidProtocol;
  }
  return ProviderAdmissionFailure::InvalidProtocol;
}

auto ProviderClosure::admit(
    const std::string& path,
    ProviderAdmissionFailure& failure) -> bool {
  if (std::find_if(
          loaded.begin(), loaded.end(), [&](const LoadedProvider& candidate) {
            return candidate.path == path;
          }) != loaded.end()) {
    failure = ProviderAdmissionFailure::DuplicateExport;
    return false;
  }

  Plugin::LoadFailure load_failure = Plugin::LoadFailure::InvalidProtocol;
  auto plugin = Plugin::Loaded::open(path, host(), load_failure);
  if (!plugin) {
    failure = translate(load_failure);
    return false;
  }

  std::vector<Plugin::ProviderDescriptor> providers;
  if (!plugin->providers(providers)) {
    plugin->close();
    failure = ProviderAdmissionFailure::InvalidProtocol;
    return false;
  }
  for (size_t index = 0; index < providers.size(); ++index) {
    for (size_t other = index + 1; other < providers.size(); ++other) {
      if (descriptor_equal(providers[index], providers[other])) {
        plugin->close();
        failure = ProviderAdmissionFailure::DuplicateExport;
        return false;
      }
    }
    std::vector<Plugin::RequirementDescriptor> dependencies;
    if (!plugin->dependencies(providers[index].candidate, dependencies)) {
      plugin->close();
      failure = ProviderAdmissionFailure::InvalidProtocol;
      return false;
    }
    for (const Plugin::RequirementDescriptor& dependency : dependencies) {
      if (find(dependency) == nullptr) {
        plugin->close();
        failure = ProviderAdmissionFailure::UndeclaredRequirement;
        return false;
      }
    }
  }

  // A later authored plugin may depend on an exact export admitted earlier in
  // the same Environment. Publishing those identities into this closure after
  // validation creates the declared provider DAG without a process registry or
  // a second lookup namespace. Reusing one descriptor would make dependency
  // resolution ambiguous, so admission rejects it before either plugin enters
  // the child Toolchain.
  std::vector<InstalledRequirement> exported;
  exported.reserve(providers.size());
  for (const Plugin::ProviderDescriptor& provider : providers) {
    Plugin::RequirementDescriptor descriptor = {
      .package_coordinate = provider.package_coordinate,
      .exported_route = provider.exported_route,
      .contract_version = provider.export_version,
      .content_sha256 = provider.content_sha256,
    };
    if (find(descriptor) != nullptr) {
      plugin->close();
      failure = ProviderAdmissionFailure::DuplicateExport;
      return false;
    }
    exported.push_back({
      .descriptor = std::move(descriptor),
      .identity = provider.candidate,
    });
  }

  loaded.emplace_back(path, std::move(*plugin), std::move(providers));
  for (InstalledRequirement& provider : exported) {
    requirements.push_back(std::move(provider));
  }
  return true;
}

auto ProviderClosure::select(
    const std::string& path,
    const Plugin::ProviderDescriptor& descriptor) const
    -> std::optional<ttx_abstract> {
  const ttx_abstract* selected = nullptr;
  for (const LoadedProvider& plugin : loaded) {
    if (plugin.path != path) {
      continue;
    }
    for (const Plugin::ProviderDescriptor& candidate : plugin.providers) {
      if (!descriptor_equal(candidate, descriptor)) {
        continue;
      }
      if (selected != nullptr) {
        return std::nullopt;
      }
      selected = &candidate.candidate;
    }
  }
  return selected == nullptr ? std::optional<ttx_abstract>()
                             : std::optional<ttx_abstract>(*selected);
}

auto ProviderClosure::providers(const std::string& path) const
    -> std::vector<Plugin::ProviderDescriptor> {
  for (const LoadedProvider& plugin : loaded) {
    if (plugin.path == path) {
      return plugin.providers;
    }
  }
  return {};
}

auto ProviderClosure::select_terminal(
    const std::string& path,
    const Plugin::ProviderDescriptor& descriptor) const
    -> std::optional<tetrodotoxin_terminal_provider> {
  const tetrodotoxin_terminal_provider* selected = nullptr;
  for (const LoadedProvider& plugin : loaded) {
    if (plugin.path != path) {
      continue;
    }
    for (const Plugin::ProviderDescriptor& candidate : plugin.providers) {
      if (!descriptor_equal(candidate, descriptor) ||
          candidate.terminal.operations == nullptr) {
        continue;
      }
      if (selected != nullptr) {
        return std::nullopt;
      }
      selected = &candidate.terminal;
    }
  }
  return selected == nullptr
             ? std::optional<tetrodotoxin_terminal_provider>()
             : std::optional<tetrodotoxin_terminal_provider>(*selected);
}

auto ProviderClosure::find_terminal(
    const std::string& path,
    const std::string& exported_route) const
    -> std::optional<tetrodotoxin_terminal_provider> {
  const tetrodotoxin_terminal_provider* selected = nullptr;
  for (const LoadedProvider& plugin : loaded) {
    if (plugin.path != path) {
      continue;
    }
    for (const Plugin::ProviderDescriptor& candidate : plugin.providers) {
      if (candidate.exported_route != exported_route ||
          candidate.terminal.operations == nullptr) {
        continue;
      }
      if (selected != nullptr) {
        return std::nullopt;
      }
      selected = &candidate.terminal;
    }
  }
  return selected == nullptr
             ? std::optional<tetrodotoxin_terminal_provider>()
             : std::optional<tetrodotoxin_terminal_provider>(*selected);
}

auto ProviderClosure::select_dialect(
    const std::string& path,
    const Plugin::ProviderDescriptor& descriptor) const
    -> std::optional<tetrodotoxin_dialect_provider> {
  const tetrodotoxin_dialect_provider* selected = nullptr;
  for (const LoadedProvider& plugin : loaded) {
    if (plugin.path != path) {
      continue;
    }
    for (const Plugin::ProviderDescriptor& candidate : plugin.providers) {
      if (!descriptor_equal(candidate, descriptor) ||
          candidate.dialect.operations == nullptr) {
        continue;
      }
      if (selected != nullptr) {
        return std::nullopt;
      }
      selected = &candidate.dialect;
    }
  }
  return selected == nullptr
             ? std::optional<tetrodotoxin_dialect_provider>()
             : std::optional<tetrodotoxin_dialect_provider>(*selected);
}

auto ProviderClosure::find_dialect(
    const std::string& path,
    const std::string& exported_route) const
    -> std::optional<tetrodotoxin_dialect_provider> {
  const tetrodotoxin_dialect_provider* selected = nullptr;
  for (const LoadedProvider& plugin : loaded) {
    if (plugin.path != path) {
      continue;
    }
    for (const Plugin::ProviderDescriptor& candidate : plugin.providers) {
      if (candidate.exported_route != exported_route ||
          candidate.dialect.operations == nullptr) {
        continue;
      }
      if (selected != nullptr) {
        return std::nullopt;
      }
      selected = &candidate.dialect;
    }
  }
  return selected == nullptr
             ? std::optional<tetrodotoxin_dialect_provider>()
             : std::optional<tetrodotoxin_dialect_provider>(*selected);
}
