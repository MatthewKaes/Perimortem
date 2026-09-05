// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <atomic>
#include <cstring>
#include <utility>
#include <vector>

#include "tetrodotoxin/environment/provider.h"
#include "tetrodotoxin/language/error.hpp"
#include "tetrodotoxin/plugin/abi.h"
#include "tetrodotoxin/terminal/artifact_request.hpp"
#include "tetrodotoxin/terminal/graph_text.hpp"
#include "tetrodotoxin/terminal/provider.h"
#include "ttx/query.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;

static std::atomic<uint64_t> plugin_references = 1;
static bool plugin_closed = false;
static ttx_abstract terminal_requirement = {};

static auto borrowed(const char* text) -> ttx_borrowed_bytes {
  return {
    .data = reinterpret_cast<const uint8_t*>(text),
    .size = std::strlen(text),
  };
}

class Provider final : public Ttx::Abstract {
 public:
  auto name() const -> ttx_borrowed_bytes override {
    return borrowed("Graph Text");
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

class GraphTextError final : public Language::Error {
 public:
  explicit GraphTextError(Core::View::Bytes message) : message(message) {}

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
  GraphTextError error(message);
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
    tetrodotoxin_workspace_view workspace,
    ttx_abstract,
    ttx_abstract,
    tetrodotoxin_terminal_begin_result result) {
  if (output_route.data == nullptr || output_route.size == 0 ||
      product.operations == nullptr || workspace.operations == nullptr ||
      workspace.self == nullptr ||
      workspace.operations->header.abi_major != TTX_ABI_MAJOR ||
      workspace.operations->header.size <
          sizeof(tetrodotoxin_workspace_view_ops) ||
      workspace.operations->root == nullptr) {
    fail(
        result,
        "Graph Text production requires an output route, product, and graph."_view);
    return;
  }

  std::vector<uint8_t> route(
      output_route.data, output_route.data + output_route.size);
  const ttx_abstract graph = workspace.operations->root(workspace.self);
  if (graph.operations == nullptr) {
    fail(result, "Graph Text received an invalid Workspace root."_view);
    return;
  }
  std::vector<uint8_t> contents =
      Terminal::GraphText::write({}, provider.get_abi(), product, graph);
  auto request = Terminal::ArtifactRequest::create(
      std::move(route), std::move(contents), false, workspace);
  if (!request) {
    fail(result, "Graph Text product support could not be retained."_view);
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
static constexpr uint8_t provider_package[] = "Tetrodotoxin.GraphText";
static constexpr uint8_t provider_route[] = "Graph";
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
  .content_sha256 = {0x47},
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
    GraphTextError error("Graph Text provider is closed."_view);
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
    GraphTextError error("Graph Text provider candidate is invalid."_view);
    result.operations->failed(result.self, error.get_handle());
    return;
  }
  result.operations->requirement(result.self, &requirement_descriptor);
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

extern "C" TTX_EXPORT void TTX_CALL
    ttx_plugin_entry(ttx_host_requirements host, ttx_plugin_entry_sink result) {
  RequirementCapture capture = {};
  host.operations->resolve(
      host.self, &requirement_descriptor,
      {
        .operations = &requirement_result_operations,
        .self = reinterpret_cast<ttx_requirement_result_self*>(&capture),
      });
  if (!capture.answered || capture.requirement.operations == nullptr) {
    GraphTextError error(
        "Host did not supply the Terminal provider contract."_view);
    result.operations->failed(result.self, error.get_handle());
    return;
  }
  terminal_requirement = capture.requirement;
  plugin_closed = false;
  result.operations->opened(
      result.self,
      {
        .operations = &plugin_operations,
        .self = reinterpret_cast<ttx_plugin_self*>(&plugin_references),
      });
}
