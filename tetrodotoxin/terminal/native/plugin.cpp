// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <atomic>
#include <cstring>
#include <utility>
#include <vector>

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/environment/provider.h"
#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/language/error.hpp"
#include "tetrodotoxin/plugin/abi.h"
#include "tetrodotoxin/terminal/artifact_request.hpp"
#include "tetrodotoxin/terminal/native/compiler.hpp"
#include "tetrodotoxin/terminal/provider.h"
#include "ttx/query.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;

static std::atomic<uint64_t> plugin_references = 1;
static bool plugin_closed = false;
static ttx_abstract terminal_requirement = {};
static ttx_abstract workspace_requirement = {};
static ttx_abstract native_toolchain_requirement = {};
static ttx_abstract cpp_owner_requirement = {};

static auto borrowed(const char* text) -> ttx_borrowed_bytes {
  return {
    .data = reinterpret_cast<const uint8_t*>(text),
    .size = std::strlen(text),
  };
}

static auto proves(ttx_abstract candidate, ttx_abstract requirement) -> bool {
  const ttx_interface_relation relation = Ttx::relation(candidate, requirement);
  return relation == TTX_INTERFACE_SATISFIED ||
         relation == TTX_INTERFACE_EQUIVALENT;
}

static auto local_owner(ttx_abstract candidate)
    -> Core::Option<const Ttx::Concept::Abstract&> {
  // A host supplied SDK identity proves this optional optimization before the
  // plugin recovers private C++ dispatch. Foreign owners continue through the
  // portable Terminal contract and are never interpreted as native objects
  // because their operation tables happen to look familiar.
  Ttx::Abstract* owner =
      Ttx::Abstract::local_owner(candidate, cpp_owner_requirement);
  return owner == nullptr
             ? Core::Option<const Ttx::Concept::Abstract&>()
             : Core::Option<const Ttx::Concept::Abstract&>(
                   static_cast<const Ttx::Concept::Abstract&>(*owner));
}

class Provider final : public Ttx::Abstract {
 public:
  auto name() const -> ttx_borrowed_bytes override {
    return borrowed("Native executable");
  }

 protected:
  auto negotiate(ttx_abstract requirement) const
      -> ttx_interface_relation override {
    return terminal_requirement.operations != nullptr &&
                   ttx_abstract_same(requirement, terminal_requirement)
               ? TTX_INTERFACE_SATISFIED
               : Ttx::Abstract::negotiate(requirement);
  }
};

class NativeError final : public Language::Error {
 public:
  explicit NativeError(Core::View::Bytes message) : message(message) {}

  constexpr auto get_name() const -> Core::View::Bytes override {
    return message;
  }
  TTX_EMPTY_DOCUMENTATION();

  void describe(Ttx::Lexical::Errors::Report& report) const override {
    report << message;
  }

 private:
  Core::View::Bytes message;
};

static Provider provider;

static void fail(
    tetrodotoxin_terminal_begin_result result,
    Core::View::Bytes message) {
  NativeError error(message);
  result.operations->failed(result.self, error.get_handle());
}

static void TTX_CALL terminal_retain(tetrodotoxin_terminal_provider_self*) {
  plugin_references.fetch_add(1, std::memory_order_relaxed);
}

static void TTX_CALL terminal_release(tetrodotoxin_terminal_provider_self*) {
  plugin_references.fetch_sub(1, std::memory_order_acq_rel);
}

static auto TTX_CALL terminal_candidate(tetrodotoxin_terminal_provider_self*)
    -> ttx_abstract {
  return provider.get_abi();
}

static void TTX_CALL terminal_begin(
    tetrodotoxin_terminal_provider_self*,
    ttx_borrowed_bytes output_route,
    ttx_abstract product,
    tetrodotoxin_workspace_view workspace_view,
    ttx_abstract environment,
    ttx_abstract invocation,
    tetrodotoxin_terminal_begin_result result) {
  if (output_route.data == nullptr || output_route.size == 0 ||
      product.operations == nullptr || workspace_view.operations == nullptr ||
      workspace_view.self == nullptr ||
      workspace_view.operations->header.abi_major != TTX_ABI_MAJOR ||
      workspace_view.operations->header.size <
          sizeof(tetrodotoxin_workspace_view_ops) ||
      workspace_view.operations->root == nullptr ||
      environment.operations == nullptr || invocation.operations == nullptr ||
      workspace_requirement.operations == nullptr ||
      native_toolchain_requirement.operations == nullptr ||
      !proves(environment, native_toolchain_requirement)) {
    fail(
        result,
        "Native production requires an exact product, Workspace, Environment toolchain, and Invocation."_view);
    return;
  }

  const ttx_abstract graph =
      workspace_view.operations->root(workspace_view.self);
  if (graph.operations == nullptr || !proves(graph, workspace_requirement)) {
    fail(result, "Native production received an invalid Workspace root."_view);
    return;
  }
  auto local_graph = local_owner(graph);
  auto workspace = local_graph ? local_graph->select<Environment::Workspace>()
                               : Core::Option<const Environment::Workspace&>();
  const ttx_abstract resolved_product = Ttx::resolve(product);
  auto local_product = local_owner(resolved_product);
  auto local_environment = local_owner(environment);
  auto toolchain =
      local_environment
          ? local_environment->select<Terminal::Native::Toolchain>()
          : Core::Option<const Terminal::Native::Toolchain&>();
  if (!workspace || !local_product || !toolchain) {
    fail(
        result,
        "Native production cannot use the optional C++ fast path for this provider boundary."_view);
    return;
  }

  auto package = workspace->find_package_owner(*local_product);
  auto library =
      package ? package->get_library().get_language().select<Library::Dialect>()
              : Core::Option<const Library::Dialect&>();
  if (!package || !library) {
    fail(
        result,
        "Native production could not recover the package and Library owners for this export."_view);
    return;
  }

  Memory::Allocator::Arena arena;
  Ttx::Lexical::Errors errors;
  auto compiled = Terminal::Native::Compiler().compile(
      arena, *workspace, *package, *local_product, *library, *toolchain,
      errors);
  Core::Option<const Memory::Dynamic::Bytes&> bytes;
  compiled.visit(
      [&](const Memory::Dynamic::Bytes& value) { bytes = value; },
      [](Terminal::Native::Failure) {});
  if (!bytes || !errors.is_empty()) {
    fail(result, "Native executable production failed."_view);
    return;
  }

  std::vector<uint8_t> route(
      output_route.data, output_route.data + output_route.size);
  const Core::View::Bytes compiled_bytes = bytes->get_view();
  std::vector<uint8_t> contents(
      compiled_bytes.get_data(),
      compiled_bytes.get_data() + compiled_bytes.get_size());
  auto request = Terminal::ArtifactRequest::create(
      std::move(route), std::move(contents), true, workspace_view);
  if (!request) {
    fail(
        result,
        "Native executable product support could not be retained."_view);
    return;
  }
  result.operations->requested(result.self, *request);
}

static const tetrodotoxin_terminal_provider_ops terminal_operations = {
  .header =
      {
        .size = sizeof(tetrodotoxin_terminal_provider_ops),
        .abi_major = TTX_ABI_MAJOR,
        .abi_minor = TTX_ABI_MINOR,
      },
  .retain = terminal_retain,
  .release = terminal_release,
  .candidate = terminal_candidate,
  .begin = terminal_begin,
};

static auto terminal_handle() -> tetrodotoxin_terminal_provider {
  return {
    .operations = &terminal_operations,
    .self = reinterpret_cast<tetrodotoxin_terminal_provider_self*>(
        &plugin_references),
  };
}

static constexpr uint8_t requirement_package[] = "Tetrodotoxin.Terminal";
static constexpr uint8_t requirement_route[] = "Provider";
static constexpr uint8_t requirement_version[] = "1.0";
static constexpr uint8_t workspace_requirement_package[] =
    "Tetrodotoxin.Environment";
static constexpr uint8_t workspace_requirement_route[] = "Workspace";
static constexpr uint8_t workspace_requirement_version[] = "1.0";
static constexpr uint8_t native_requirement_package[] =
    "Tetrodotoxin.Terminal.Native";
static constexpr uint8_t native_requirement_route[] = "Toolchain";
static constexpr uint8_t native_requirement_version[] = "1.0";
static constexpr uint8_t cpp_owner_requirement_package[] = "Tetrodotoxin.Cpp";
static constexpr uint8_t cpp_owner_requirement_route[] = "Owner";
static constexpr uint8_t cpp_owner_requirement_version[] = "1.0";
static constexpr uint8_t provider_package[] = "Tetrodotoxin.LLVM";
static constexpr uint8_t provider_route[] = "NativeExecutable";
static constexpr uint8_t provider_version[] = "1.0";

static const ttx_requirement_descriptor requirement_descriptor = {
  .header =
      {
        .size = sizeof(ttx_requirement_descriptor),
        .abi_major = TTX_PLUGIN_ABI_MAJOR,
        .abi_minor = TTX_PLUGIN_ABI_MINOR,
      },
  .package_coordinate = {requirement_package, sizeof(requirement_package) - 1},
  .exported_route = {requirement_route, sizeof(requirement_route) - 1},
  .contract_version = {requirement_version, sizeof(requirement_version) - 1},
  .content_sha256 = {0x54},
};

static const ttx_requirement_descriptor workspace_requirement_descriptor = {
  .header =
      {
        .size = sizeof(ttx_requirement_descriptor),
        .abi_major = TTX_PLUGIN_ABI_MAJOR,
        .abi_minor = TTX_PLUGIN_ABI_MINOR,
      },
  .package_coordinate =
      {
        workspace_requirement_package,
        sizeof(workspace_requirement_package) - 1,
      },
  .exported_route =
      {
        workspace_requirement_route,
        sizeof(workspace_requirement_route) - 1,
      },
  .contract_version =
      {
        workspace_requirement_version,
        sizeof(workspace_requirement_version) - 1,
      },
  .content_sha256 = {0x57},
};

static const ttx_requirement_descriptor native_requirement_descriptor = {
  .header =
      {
        .size = sizeof(ttx_requirement_descriptor),
        .abi_major = TTX_PLUGIN_ABI_MAJOR,
        .abi_minor = TTX_PLUGIN_ABI_MINOR,
      },
  .package_coordinate =
      {
        native_requirement_package,
        sizeof(native_requirement_package) - 1,
      },
  .exported_route =
      {
        native_requirement_route,
        sizeof(native_requirement_route) - 1,
      },
  .contract_version =
      {
        native_requirement_version,
        sizeof(native_requirement_version) - 1,
      },
  .content_sha256 = {0x4c},
};

static const ttx_requirement_descriptor cpp_owner_requirement_descriptor = {
  .header =
      {
        .size = sizeof(ttx_requirement_descriptor),
        .abi_major = TTX_PLUGIN_ABI_MAJOR,
        .abi_minor = TTX_PLUGIN_ABI_MINOR,
      },
  .package_coordinate =
      {
        cpp_owner_requirement_package,
        sizeof(cpp_owner_requirement_package) - 1,
      },
  .exported_route =
      {
        cpp_owner_requirement_route,
        sizeof(cpp_owner_requirement_route) - 1,
      },
  .contract_version =
      {
        cpp_owner_requirement_version,
        sizeof(cpp_owner_requirement_version) - 1,
      },
  .content_sha256 = {0x43},
};

static const ttx_provider_export_descriptor provider_descriptor = {
  .header =
      {
        .size = sizeof(ttx_provider_export_descriptor),
        .abi_major = TTX_PLUGIN_ABI_MAJOR,
        .abi_minor = TTX_PLUGIN_ABI_MINOR,
      },
  .package_coordinate = {provider_package, sizeof(provider_package) - 1},
  .exported_route = {provider_route, sizeof(provider_route) - 1},
  .export_version = {provider_version, sizeof(provider_version) - 1},
  .content_sha256 = {0x4e},
};

static void TTX_CALL plugin_retain(ttx_plugin_self*) {
  plugin_references.fetch_add(1, std::memory_order_relaxed);
}

static void TTX_CALL plugin_release(ttx_plugin_self*) {
  plugin_references.fetch_sub(1, std::memory_order_acq_rel);
}

static void TTX_CALL
    visit_providers(ttx_plugin_self*, ttx_provider_sink result) {
  if (plugin_closed) {
    NativeError error("Native executable provider is closed."_view);
    result.operations->failed(result.self, error.get_handle());
    return;
  }
  result.operations->terminal(
      result.self, &provider_descriptor, terminal_handle());
  result.operations->completed(result.self);
}

static void TTX_CALL visit_dependencies(
    ttx_plugin_self*,
    ttx_abstract candidate,
    ttx_requirement_sink result) {
  if (plugin_closed || !ttx_abstract_same(candidate, provider.get_abi())) {
    NativeError error("Native executable provider candidate is invalid."_view);
    result.operations->failed(result.self, error.get_handle());
    return;
  }
  result.operations->requirement(result.self, &requirement_descriptor);
  result.operations->requirement(
      result.self, &workspace_requirement_descriptor);
  result.operations->requirement(result.self, &native_requirement_descriptor);
  result.operations->requirement(
      result.self, &cpp_owner_requirement_descriptor);
  result.operations->completed(result.self);
}

static void TTX_CALL
    plugin_close(ttx_plugin_self*, ttx_plugin_close_sink result) {
  plugin_closed = true;
  result.operations->closed(result.self);
}

static const ttx_plugin_ops plugin_operations = {
  .header =
      {
        .size = sizeof(ttx_plugin_ops),
        .abi_major = TTX_PLUGIN_ABI_MAJOR,
        .abi_minor = TTX_PLUGIN_ABI_MINOR,
      },
  .retain = plugin_retain,
  .release = plugin_release,
  .visit_providers = visit_providers,
  .visit_dependencies = visit_dependencies,
  .close = plugin_close,
};

struct RequirementCapture {
  bool answered;
  ttx_abstract requirement;
};

static void TTX_CALL requirement_resolved(
    ttx_requirement_result_self* self,
    ttx_abstract requirement) {
  auto& capture = *reinterpret_cast<RequirementCapture*>(self);
  capture.answered = true;
  capture.requirement = requirement;
}

static void TTX_CALL requirement_missing(ttx_requirement_result_self* self) {
  reinterpret_cast<RequirementCapture*>(self)->answered = true;
}

static void TTX_CALL requirement_incompatible(
    ttx_requirement_result_self* self,
    const ttx_requirement_descriptor*) {
  requirement_missing(self);
}

static void TTX_CALL
    requirement_failed(ttx_requirement_result_self* self, ttx_abstract) {
  requirement_missing(self);
}

static const ttx_requirement_result_ops requirement_result_operations = {
  .header =
      {
        .size = sizeof(ttx_requirement_result_ops),
        .abi_major = TTX_PLUGIN_ABI_MAJOR,
        .abi_minor = TTX_PLUGIN_ABI_MINOR,
      },
  .resolved = requirement_resolved,
  .missing = requirement_missing,
  .incompatible = requirement_incompatible,
  .failed = requirement_failed,
};

static auto resolve_host_requirement(
    ttx_host_requirements host,
    const ttx_requirement_descriptor& descriptor) -> ttx_abstract {
  RequirementCapture capture = {};
  host.operations->resolve(
      host.self, &descriptor,
      {
        .operations = &requirement_result_operations,
        .self = reinterpret_cast<ttx_requirement_result_self*>(&capture),
      });
  return capture.answered ? capture.requirement : ttx_abstract{};
}

extern "C" TTX_EXPORT void TTX_CALL
    ttx_plugin_entry(ttx_host_requirements host, ttx_plugin_entry_sink result) {
  terminal_requirement = resolve_host_requirement(host, requirement_descriptor);
  workspace_requirement =
      resolve_host_requirement(host, workspace_requirement_descriptor);
  native_toolchain_requirement =
      resolve_host_requirement(host, native_requirement_descriptor);
  cpp_owner_requirement =
      resolve_host_requirement(host, cpp_owner_requirement_descriptor);
  if (terminal_requirement.operations == nullptr ||
      workspace_requirement.operations == nullptr ||
      native_toolchain_requirement.operations == nullptr ||
      cpp_owner_requirement.operations == nullptr) {
    NativeError error(
        "Host did not supply the Native provider's complete contract closure."_view);
    result.operations->failed(result.self, error.get_handle());
    return;
  }
  plugin_closed = false;
  result.operations->opened(
      result.self,
      {
        .operations = &plugin_operations,
        .self = reinterpret_cast<ttx_plugin_self*>(&plugin_references),
      });
}
