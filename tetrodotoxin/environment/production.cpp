// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/production.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/system/path.hpp"

#include "tetrodotoxin/environment/dialect.hpp"
#include "tetrodotoxin/environment/provider.h"
#include "tetrodotoxin/environment/provider_closure.hpp"
#include "tetrodotoxin/environment/sdk.hpp"
#include "tetrodotoxin/environment/toolchain.hpp"
#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/language/artifact_file.hpp"
#include "tetrodotoxin/language/error.hpp"
#include "tetrodotoxin/language/product.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "tetrodotoxin/package/repository/repository.hpp"
#include "tetrodotoxin/terminal/native/provider.h"
#include "tetrodotoxin/terminal/native/toolchain.hpp"
#include "tetrodotoxin/terminal/provider.h"
#include "tetrodotoxin/terminal/query.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/layouts/named.hpp"
#include "ttx/query.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;

class EnvironmentProductionError final : public Language::Error {
 public:
  explicit EnvironmentProductionError(Core::View::Bytes message)
      : message(message) {}

  TTX_NAME("Environment production error"_view);
  TTX_EMPTY_DOCUMENTATION();

  void describe(Ttx::Lexical::Errors::Report& report) const override {
    report << message;
  }

 private:
  Core::View::Bytes message;
};

class EnvironmentBuildLifetime final : public Language::ProductionLifetime {
 public:
  explicit EnvironmentBuildLifetime(
      std::vector<Environment::InstalledRequirement> requirements)
      : providers(std::move(requirements)) {}

  // Destruction runs in reverse declaration order. Requests leave the owning
  // Production first, then Workspace and Toolchain release every borrowed
  // provider before ProviderClosure closes and unmaps the plugin libraries.
  Environment::ProviderClosure providers;
  Environment::Toolchain toolchain;
  Memory::Allocator::Arena arena;
  Package::Repository::Repository* repository = nullptr;
  std::unique_ptr<Environment::Workspace> workspace;
  std::vector<std::pair<std::string, std::string>> loaded;
  std::vector<std::string> include_storage;
  std::vector<std::string> archive_storage;
  std::vector<std::string> option_storage;
  std::vector<Core::View::Bytes> includes;
  std::vector<Core::View::Bytes> archives;
  std::vector<Core::View::Bytes> options;
  std::unique_ptr<Terminal::Native::Toolchain> native;
};

static void release_requests(
    std::vector<tetrodotoxin_product_request>& requests) {
  for (tetrodotoxin_product_request request : requests) {
    request.operations->release(request.self);
  }
  requests.clear();
}

static void fail(
    Memory::Allocator::Arena& arena,
    Core::View::Bytes message,
    tetrodotoxin_production_result result) {
  auto& error =
      arena.construct<EnvironmentProductionError>(arena.proxy(message));
  result.operations->failed(result.self, error.get_handle());
}

static auto text(Core::View::Bytes value) -> std::string {
  return std::string(
      reinterpret_cast<const char*>(value.get_data()), value.get_size());
}

static auto text(const std::vector<uint8_t>& value) -> std::string {
  return std::string(reinterpret_cast<const char*>(value.data()), value.size());
}

static auto join(const std::string& root, const std::string& child)
    -> std::string {
  return root.empty() || root.back() == '/' ? root + child : root + "/" + child;
}

static auto requirement(const char* package, const char* route, uint8_t digest)
    -> Plugin::RequirementDescriptor {
  Plugin::RequirementDescriptor descriptor = {
    .package_coordinate = package,
    .exported_route = route,
    .contract_version = "1.0",
    .content_sha256 = {},
  };
  descriptor.content_sha256[0] = digest;
  return descriptor;
}

static auto local_dialect(
    tetrodotoxin_dialect_provider provider,
    ttx_abstract cpp_owner_requirement) -> Core::Option<Language::Dialect&> {
  const ttx_abstract candidate = provider.operations->candidate(provider.self);
  const ttx_interface_relation dialect =
      Ttx::relation(candidate, tetrodotoxin_dialect_requirement());
  if ((dialect != TTX_INTERFACE_SATISFIED &&
       dialect != TTX_INTERFACE_EQUIVALENT) ||
      Ttx::relation(candidate, cpp_owner_requirement) !=
          TTX_INTERFACE_SATISFIED) {
    return {};
  }
  Ttx::Abstract* owner =
      Ttx::Abstract::local_owner(candidate, cpp_owner_requirement);
  if (owner == nullptr) {
    return {};
  }
  auto& abstract = static_cast<Ttx::Concept::Abstract&>(*owner);
  return static_cast<Language::Dialect&>(abstract);
}

static auto append_views(
    const std::vector<std::string>& values,
    std::vector<Core::View::Bytes>& views) -> void {
  views.reserve(values.size());
  for (const std::string& value : values) {
    views.emplace_back(
        reinterpret_cast<const uint8_t*>(value.data()), value.size());
  }
}

void Environment::Dialect::produce_build(
    const Plan& plan,
    Core::View::Bytes build_source,
    Core::View::Bytes package_locator,
    Core::View::Vector<ProductDescription> products,
    const Ttx::Concept::Abstract& graph,
    ttx_context,
    Memory::Allocator::Arena& result_arena,
    tetrodotoxin_production_result result) const {
  auto bootstrap_workspace = graph.select<Environment::Workspace>();
  if (!bootstrap_workspace || bootstrap_workspace->get_outer() == nullptr) {
    fail(
        result_arena,
        "Environment requires the Build's bootstrap Workspace and Invocation."_view,
        result);
    return;
  }
  Ttx::Concept::Abstract& invocation = *bootstrap_workspace->get_outer();
  auto sdk_value =
      Ttx::copy_bytes(invocation.resolve_concept("sdk"_view).get_handle());
  if (!sdk_value || sdk_value->empty()) {
    fail(
        result_arena,
        "Environment could not resolve the installed Puffer SDK root."_view,
        result);
    return;
  }
  const std::string sdk = text(*sdk_value);
  auto sdk_manifest = Environment::Sdk::open(sdk);
  if (!sdk_manifest) {
    fail(
        result_arena,
        "Environment could not read the installed SDK provider manifest."_view,
        result);
    return;
  }

  const ttx_abstract cpp_owner_requirement =
      Ttx::Concept::Abstract::owner_requirement();
  auto lifetime = std::make_unique<EnvironmentBuildLifetime>(
      std::vector<Environment::InstalledRequirement>{
        {
          .descriptor = requirement("Tetrodotoxin.Terminal", "Provider", 0x54),
          .identity = tetrodotoxin_terminal_requirement(),
        },
        {
          .descriptor =
              requirement("Tetrodotoxin.Environment", "Workspace", 0x57),
          .identity = tetrodotoxin_workspace_requirement(),
        },
        {
          .descriptor =
              requirement("Tetrodotoxin.Terminal.Native", "Toolchain", 0x4c),
          .identity = tetrodotoxin_native_toolchain_requirement(),
        },
        {
          .descriptor = requirement("Tetrodotoxin.Cpp", "Owner", 0x43),
          .identity = cpp_owner_requirement,
        },
      });

  lifetime->loaded.reserve(plan.plugins.get_size());
  Environment::ProviderAdmissionFailure admission =
      Environment::ProviderAdmissionFailure::InvalidProtocol;
  for (Core::View::Bytes artifact : plan.plugins.get_view()) {
    auto path = sdk_manifest->select(text(artifact));
    if (!path || !lifetime->providers.admit(*path, admission)) {
      fail(
          result_arena,
          "Environment could not admit one explicitly requested SDK plugin."_view,
          result);
      return;
    }
    lifetime->loaded.emplace_back(text(artifact), std::move(*path));
  }

  Library::Dialect* library = nullptr;
  for (const auto& [artifact, path] : lifetime->loaded) {
    auto provider = lifetime->providers.find_dialect(path, "Dialect");
    if (!provider) {
      continue;
    }
    auto dialect = local_dialect(*provider, cpp_owner_requirement);
    if (!dialect || !lifetime->toolchain.install(*provider, *dialect)) {
      fail(
          result_arena,
          "Environment could not install one selected Dialect provider."_view,
          result);
      return;
    }
    if (artifact == "Tetrodotoxin.Library") {
      library = &static_cast<Library::Dialect&>(*dialect);
    }
  }
  if (library == nullptr || !lifetime->toolchain.install<Package::Dialect>(
                                "Package"_view, *library)) {
    fail(
        result_arena,
        "Environment requires the selected Library Dialect before Package bootstrap."_view,
        result);
    return;
  }

  const std::string repository_root = join(sdk, "packages/ttx");
  auto repository = Package::Repository::Repository::create(
      lifetime->arena, Core::NullTerminated::to_view(repository_root.c_str()),
      Core::NullTerminated::to_view(repository_root.c_str()));
  if (!repository) {
    fail(
        result_arena,
        "Environment could not open the selected SDK package repository."_view,
        result);
    return;
  }
  lifetime->repository = &*repository;

  lifetime->workspace = std::make_unique<Environment::Workspace>(
      lifetime->toolchain,
      Perimortem::Core::Option<
          Perimortem::Memory::Dynamic::Record<Package::Snapshots>>(),
      lifetime->repository, &invocation);
  Ttx::Lexical::Errors errors;
  Perimortem::System::Path source_path(build_source);
  Core::View::Bytes package_root = source_path.get_directory();
  if (package_root.is_empty()) {
    package_root = "."_view;
  }
  auto imported = lifetime->workspace->import_package(
      errors, package_root, "Product"_view, package_locator);
  auto package = imported ? imported->select<Package::Language::Monograph>()
                          : Core::Option<Package::Language::Monograph&>();
  if (!package || !errors.is_empty()) {
    fail(
        result_arena,
        "Environment could not construct the Build's live package graph."_view,
        result);
    return;
  }

  lifetime->include_storage = {sdk};
  lifetime->archive_storage = {
    join(sdk, ".bin/bin/perimortem/libcore.a"),
    join(sdk, ".bin/bin/perimortem/libmemory.a"),
    join(sdk, ".bin/bin/perimortem/libcompression.a"),
    join(sdk, ".bin/bin/perimortem/libserialization.a"),
    join(sdk, ".bin/bin/perimortem/libsystem.a"),
    join(sdk, ".bin/bin/perimortem/libgraphics.a"),
    join(sdk, ".bin/bin/perimortem/libvulkan.a"),
    join(sdk, ".bin/bin/perimortem/libplatform.a"),
    join(sdk, ".bin/bin/tetrodotoxin/graphics/libgraphics.a"),
    join(sdk, ".bin/bin/tetrodotoxin/runtime/libapplication.a"),
  };
  lifetime->option_storage = {
    "-lvulkan", "-lwayland-client", "-lpthread", "-ldl", "-lm",
  };
  append_views(lifetime->include_storage, lifetime->includes);
  append_views(lifetime->archive_storage, lifetime->archives);
  append_views(lifetime->option_storage, lifetime->options);
  lifetime->native = std::make_unique<Terminal::Native::Toolchain>(
      "/usr/bin/clang++"_view,
      Core::View::Vector<Core::View::Bytes>(
          lifetime->includes.data(), lifetime->includes.size()),
      Core::View::Vector<Core::View::Bytes>(
          lifetime->archives.data(), lifetime->archives.size()),
      Core::View::Vector<Core::View::Bytes>(
          lifetime->options.data(), lifetime->options.size()));

  std::vector<tetrodotoxin_product_request> requests;
  for (const ProductDescription& requested : products) {
    const Ttx::Concept::Abstract& product =
        package->resolve_concept(requested.package_route);
    if (ttx_abstract_same(product.get_handle(), ttx_unknown())) {
      release_requests(requests);
      result.operations->unknown(result.self);
      return;
    }
    if (ttx_abstract_same(product.get_handle(), ttx_none())) {
      release_requests(requests);
      fail(
          result_arena,
          "Build selected a package route that did not resolve."_view, result);
      return;
    }
    std::string plugin_path;
    for (const auto& [artifact, path] : lifetime->loaded) {
      if (artifact == text(requested.terminal_artifact)) {
        plugin_path = path;
        break;
      }
    }
    auto terminal = plugin_path.empty()
                        ? std::optional<tetrodotoxin_terminal_provider>()
                        : lifetime->providers.find_terminal(
                              plugin_path, text(requested.terminal_export));
    if (!terminal) {
      release_requests(requests);
      fail(
          result_arena,
          "Build's Terminal locator did not select one exact provider export."_view,
          result);
      return;
    }

    const ttx_borrowed_bytes output_route = {
      .data = requested.name.get_data(),
      .size = requested.name.get_size(),
    };
    const Terminal::BeginObservation begun = Terminal::begin(
        *terminal, output_route, product.get_handle(),
        lifetime->workspace->get_provider_handle(),
        lifetime->native->get_handle(), invocation.get_handle());
    if (begun.state != Terminal::BeginState::Requested) {
      release_requests(requests);
      if (begun.state == Terminal::BeginState::Failed &&
          Language::Error::recognizes(begun.error)) {
        result.operations->failed(result.self, begun.error);
        return;
      }
      fail(
          result_arena,
          "Selected Terminal rejected the Build product request."_view, result);
      return;
    }
    requests.push_back(begun.request);
  }

  const Core::View::Bytes output_root = plan.output_root;
  auto production = Language::Production::create(
      std::vector<uint8_t>(
          output_root.get_data(),
          output_root.get_data() + output_root.get_size()),
      std::move(requests), std::move(lifetime));
  if (!production) {
    fail(
        result_arena,
        "Environment could not retain the Build production plan."_view, result);
    return;
  }
  result.operations->planned(result.self, *production);
}
