// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <string.h>

#include "tetrodotoxin/plugin/abi.h"

static const uint8_t requirement_package[] = "Validation.Contracts";
static const uint8_t requirement_route[] = "Echo";
static const uint8_t requirement_version[] = "1";
static const uint8_t provider_package[] = "Validation.Plugin";
static const uint8_t provider_route[] = "EchoProvider";
static const uint8_t terminal_route[] = "TerminalProvider";
static const uint8_t dialect_route[] = "DialectProvider";
static const uint8_t provider_version[] = "1";
static const uint8_t provider_name[] = "C plugin provider";

static ttx_abstract echo_requirement;
static uint64_t plugin_references;
static uint64_t source_graph_references;
static uint8_t plugin_closed;

static ttx_borrowed_bytes view(const uint8_t* data, uint64_t size) {
  ttx_borrowed_bytes result = {.data = data, .size = size};
  return result;
}

static const ttx_requirement_descriptor echo_requirement_descriptor = {
  .header =
      {
        .size = sizeof(ttx_requirement_descriptor),
        .abi_major = TTX_PLUGIN_ABI_MAJOR,
        .abi_minor = TTX_PLUGIN_ABI_MINOR,
      },
  .package_coordinate =
      {
        .data = requirement_package,
        .size = sizeof(requirement_package) - 1,
      },
  .exported_route =
      {
        .data = requirement_route,
        .size = sizeof(requirement_route) - 1,
      },
  .contract_version =
      {
        .data = requirement_version,
        .size = sizeof(requirement_version) - 1,
      },
  .content_sha256 = {1},
};

static const ttx_provider_export_descriptor echo_provider_descriptor = {
  .header =
      {
        .size = sizeof(ttx_provider_export_descriptor),
        .abi_major = TTX_PLUGIN_ABI_MAJOR,
        .abi_minor = TTX_PLUGIN_ABI_MINOR,
      },
  .package_coordinate =
      {
        .data = provider_package,
        .size = sizeof(provider_package) - 1,
      },
  .exported_route =
      {
        .data = provider_route,
        .size = sizeof(provider_route) - 1,
      },
  .export_version =
      {
        .data = provider_version,
        .size = sizeof(provider_version) - 1,
      },
  .content_sha256 = {2},
};

static const ttx_provider_export_descriptor terminal_provider_descriptor = {
  .header =
      {
        .size = sizeof(ttx_provider_export_descriptor),
        .abi_major = TTX_PLUGIN_ABI_MAJOR,
        .abi_minor = TTX_PLUGIN_ABI_MINOR,
      },
  .package_coordinate =
      {
        .data = provider_package,
        .size = sizeof(provider_package) - 1,
      },
  .exported_route =
      {
        .data = terminal_route,
        .size = sizeof(terminal_route) - 1,
      },
  .export_version =
      {
        .data = provider_version,
        .size = sizeof(provider_version) - 1,
      },
  .content_sha256 = {3},
};

static const ttx_provider_export_descriptor dialect_provider_descriptor = {
  .header =
      {
        .size = sizeof(ttx_provider_export_descriptor),
        .abi_major = TTX_PLUGIN_ABI_MAJOR,
        .abi_minor = TTX_PLUGIN_ABI_MINOR,
      },
  .package_coordinate =
      {
        .data = provider_package,
        .size = sizeof(provider_package) - 1,
      },
  .exported_route =
      {
        .data = dialect_route,
        .size = sizeof(dialect_route) - 1,
      },
  .export_version =
      {
        .data = provider_version,
        .size = sizeof(provider_version) - 1,
      },
  .content_sha256 = {4},
};

static uint64_t TTX_CALL documentation_size(ttx_documentation self) {
  (void)self;
  return 0;
}

static void TTX_CALL
    documentation_visit(ttx_documentation self, ttx_bytes_sink sink) {
  (void)self;
  sink.operations->completed(sink);
}

static const ttx_documentation_ops empty_documentation_operations = {
  .header =
      {
        .size = sizeof(ttx_documentation_ops),
        .abi_major = TTX_ABI_MAJOR,
        .abi_minor = TTX_ABI_MINOR,
      },
  .size = documentation_size,
  .visit_bytes = documentation_visit,
};

static ttx_borrowed_bytes TTX_CALL provider_abstract_name(ttx_abstract self) {
  (void)self;
  return view(provider_name, sizeof(provider_name) - 1);
}

static ttx_documentation TTX_CALL
    provider_abstract_documentation(ttx_abstract self) {
  return (ttx_documentation){
    .operations = &empty_documentation_operations,
    .self = (ttx_documentation_self*)self.self,
  };
}

static void TTX_CALL
    provider_abstract_resolve(ttx_abstract self, ttx_abstract_sink result) {
  result.operations->answer(result, self);
}

static void TTX_CALL provider_abstract_resolve_concept(
    ttx_abstract self,
    ttx_borrowed_bytes route,
    ttx_abstract_sink result) {
  (void)self;
  (void)route;
  result.operations->answer(result, echo_requirement);
}

static void TTX_CALL provider_abstract_visit_concepts(
    ttx_abstract self,
    ttx_concept_sink result) {
  (void)self;
  result.operations->completed(result);
}

struct provider_interface_state {
  ttx_abstract requirement;
  ttx_abstract candidate;
  ttx_interface_relation relation;
};

static struct provider_interface_state* provider_interface_state(
    ttx_interface self) {
  return (struct provider_interface_state*)self.self;
}

static ttx_abstract TTX_CALL
    provider_interface_requirement(ttx_interface self) {
  return provider_interface_state(self)->requirement;
}

static ttx_abstract TTX_CALL provider_interface_candidate(ttx_interface self);

static ttx_interface_relation TTX_CALL
    provider_interface_negotiate(ttx_interface self) {
  return provider_interface_state(self)->relation;
}

static void TTX_CALL provider_interface_invoke(
    ttx_interface self,
    ttx_abstract operation,
    ttx_pack input,
    ttx_context context,
    ttx_pack_result result) {
  (void)self;
  (void)operation;
  (void)input;
  (void)context;
  result.operations->none(result);
}

static const ttx_interface_ops provider_interface_operations = {
  .header =
      {
        .size = sizeof(ttx_interface_ops),
        .abi_major = TTX_ABI_MAJOR,
        .abi_minor = TTX_ABI_MINOR,
      },
  .requirement = provider_interface_requirement,
  .candidate = provider_interface_candidate,
  .negotiate = provider_interface_negotiate,
  .invoke = provider_interface_invoke,
};

static const ttx_abstract_ops provider_abstract_operations;

static ttx_abstract provider_abstract(void) {
  static uint8_t provider;
  return (ttx_abstract){
    .operations = &provider_abstract_operations,
    .self = (ttx_abstract_self*)&provider,
  };
}

static ttx_abstract TTX_CALL provider_interface_candidate(ttx_interface self) {
  return provider_interface_state(self)->candidate;
}

static void TTX_CALL provider_abstract_interface(
    ttx_abstract self,
    ttx_abstract requirement,
    ttx_interface_sink result) {
  struct provider_interface_state state = {
    .requirement = requirement,
    .candidate = self,
    .relation =
        ttx_abstract_same(requirement, echo_requirement) ||
                ttx_abstract_same(
                    requirement, tetrodotoxin_dialect_requirement())
            ? TTX_INTERFACE_SATISFIED
            : TTX_INTERFACE_REJECTED,
  };
  const ttx_interface answer = {
    .operations = &provider_interface_operations,
    .self = (ttx_interface_self*)&state,
  };
  result.operations->answer(result, answer);
}

static void TTX_CALL
    provider_abstract_domain(ttx_abstract self, ttx_domain_result result) {
  (void)self;
  result.operations->none(result);
}

static void TTX_CALL
    provider_abstract_callable(ttx_abstract self, ttx_callable_result result) {
  (void)self;
  result.operations->none(result);
}

static void TTX_CALL
    provider_abstract_route(ttx_abstract self, ttx_route_result result) {
  (void)self;
  result.operations->none(result);
}

static void TTX_CALL provider_abstract_extent(
    ttx_abstract self,
    ttx_finite_extent_result result) {
  (void)self;
  result.operations->none(result);
}

static void TTX_CALL
    provider_abstract_bytes(ttx_abstract self, ttx_bytes_result result) {
  (void)self;
  result.operations->none(result);
}

static const ttx_abstract_ops provider_abstract_operations = {
  .header =
      {
        .size = sizeof(ttx_abstract_ops),
        .abi_major = TTX_ABI_MAJOR,
        .abi_minor = TTX_ABI_MINOR,
      },
  .name = provider_abstract_name,
  .documentation = provider_abstract_documentation,
  .resolve = provider_abstract_resolve,
  .resolve_concept = provider_abstract_resolve_concept,
  .visit_concepts = provider_abstract_visit_concepts,
  .interface = provider_abstract_interface,
  .resolve_domain = provider_abstract_domain,
  .resolve_callable = provider_abstract_callable,
  .resolve_route = provider_abstract_route,
  .resolve_finite_extent = provider_abstract_extent,
  .resolve_bytes = provider_abstract_bytes,
};

static void TTX_CALL
    terminal_retain(tetrodotoxin_terminal_provider_self* self) {
  (void)self;
  ++plugin_references;
}

static void TTX_CALL
    terminal_release(tetrodotoxin_terminal_provider_self* self) {
  (void)self;
  if (plugin_references != 0) {
    --plugin_references;
  }
}

static ttx_abstract TTX_CALL
    terminal_candidate(tetrodotoxin_terminal_provider_self* self) {
  (void)self;
  return provider_abstract();
}

static void TTX_CALL terminal_begin(
    tetrodotoxin_terminal_provider_self* self,
    ttx_borrowed_bytes output_route,
    ttx_abstract product,
    tetrodotoxin_workspace_view workspace,
    ttx_abstract environment,
    ttx_abstract invocation,
    tetrodotoxin_terminal_begin_result result) {
  (void)self;
  (void)output_route;
  (void)product;
  (void)workspace;
  (void)environment;
  (void)invocation;
  result.operations->failed(result.self, echo_requirement);
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

static void TTX_CALL source_graph_retain(tetrodotoxin_source_graph_self* self) {
  (void)self;
  ++source_graph_references;
}

static void TTX_CALL
    source_graph_release(tetrodotoxin_source_graph_self* self) {
  (void)self;
  if (source_graph_references != 0) {
    --source_graph_references;
  }
}

static ttx_abstract TTX_CALL
    source_graph_root(tetrodotoxin_source_graph_self* self) {
  (void)self;
  return provider_abstract();
}

static ttx_abstract TTX_CALL
    source_graph_dialect(tetrodotoxin_source_graph_self* self) {
  (void)self;
  return provider_abstract();
}

static void TTX_CALL source_graph_dependencies(
    tetrodotoxin_source_graph_self* self,
    tetrodotoxin_source_dependency_sink result) {
  (void)self;
  result.operations->completed(result.self);
}

static void TTX_CALL source_graph_validate(
    tetrodotoxin_source_graph_self* self,
    tetrodotoxin_validation_result result) {
  (void)self;
  result.operations->accepted(result.self);
}

static void TTX_CALL source_graph_produce(
    tetrodotoxin_source_graph_self* self,
    ttx_context context,
    tetrodotoxin_production_result result) {
  (void)self;
  (void)context;
  result.operations->none(result.self);
}

static const tetrodotoxin_source_graph_ops source_graph_operations = {
  .header =
      {
        .size = sizeof(tetrodotoxin_source_graph_ops),
        .abi_major = TTX_ABI_MAJOR,
        .abi_minor = TTX_ABI_MINOR,
      },
  .retain = source_graph_retain,
  .release = source_graph_release,
  .root = source_graph_root,
  .dialect = source_graph_dialect,
  .visit_dependencies = source_graph_dependencies,
  .validate = source_graph_validate,
  .produce = source_graph_produce,
};

static tetrodotoxin_source_graph source_graph(void) {
  return (tetrodotoxin_source_graph){
    .operations = &source_graph_operations,
    .self = (tetrodotoxin_source_graph_self*)&source_graph_references,
  };
}

static void TTX_CALL dialect_retain(tetrodotoxin_dialect_provider_self* self) {
  (void)self;
  ++plugin_references;
}

static void TTX_CALL dialect_release(tetrodotoxin_dialect_provider_self* self) {
  (void)self;
  if (plugin_references != 0) {
    --plugin_references;
  }
}

static ttx_abstract TTX_CALL
    dialect_candidate(tetrodotoxin_dialect_provider_self* self) {
  (void)self;
  return provider_abstract();
}

static ttx_borrowed_bytes TTX_CALL
    dialect_name(tetrodotoxin_dialect_provider_self* self) {
  static const uint8_t name[] = "Fixture";
  (void)self;
  return view(name, sizeof(name) - 1);
}

static void TTX_CALL dialect_interpret(
    tetrodotoxin_dialect_provider_self* self,
    tetrodotoxin_source_input source,
    ttx_abstract context,
    tetrodotoxin_interpret_result result) {
  ttx_borrowed_bytes bytes;
  (void)self;
  (void)context;
  if (source.operations == 0 || source.operations->bytes == 0) {
    result.operations->failed(result.self, echo_requirement);
    return;
  }
  bytes = source.operations->bytes(source.self);
  if (bytes.size != 7 || bytes.data == 0 ||
      memcmp(bytes.data, "fixture", 7) != 0) {
    result.operations->failed(result.self, echo_requirement);
    return;
  }
  ++source_graph_references;
  result.operations->constructed(result.self, source_graph());
}

static const tetrodotoxin_dialect_provider_ops dialect_operations = {
  .header =
      {
        .size = sizeof(tetrodotoxin_dialect_provider_ops),
        .abi_major = TTX_ABI_MAJOR,
        .abi_minor = TTX_ABI_MINOR,
      },
  .retain = dialect_retain,
  .release = dialect_release,
  .candidate = dialect_candidate,
  .name = dialect_name,
  .interpret = dialect_interpret,
};

static void TTX_CALL plugin_retain(ttx_plugin_self* self) {
  (void)self;
  ++plugin_references;
}

static void TTX_CALL plugin_release(ttx_plugin_self* self) {
  (void)self;
  if (plugin_references != 0) {
    --plugin_references;
  }
}

static void TTX_CALL
    plugin_visit_providers(ttx_plugin_self* self, ttx_provider_sink result) {
  (void)self;
  if (plugin_closed) {
    result.operations->failed(result.self, echo_requirement);
    return;
  }
  result.operations->provider(
      result.self, &echo_provider_descriptor, provider_abstract());
  result.operations->terminal(
      result.self, &terminal_provider_descriptor,
      (tetrodotoxin_terminal_provider){
        .operations = &terminal_operations,
        .self = (tetrodotoxin_terminal_provider_self*)&plugin_references,
      });
  result.operations->dialect(
      result.self, &dialect_provider_descriptor,
      (tetrodotoxin_dialect_provider){
        .operations = &dialect_operations,
        .self = (tetrodotoxin_dialect_provider_self*)&plugin_references,
      });
  result.operations->completed(result.self);
}

static void TTX_CALL plugin_visit_dependencies(
    ttx_plugin_self* self,
    ttx_abstract candidate,
    ttx_requirement_sink result) {
  (void)self;
  if (plugin_closed || !ttx_abstract_same(candidate, provider_abstract())) {
    result.operations->failed(result.self, echo_requirement);
    return;
  }
  result.operations->requirement(result.self, &echo_requirement_descriptor);
  result.operations->completed(result.self);
}

static void TTX_CALL
    plugin_close(ttx_plugin_self* self, ttx_plugin_close_sink result) {
  (void)self;
  plugin_closed = 1;
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
  .visit_providers = plugin_visit_providers,
  .visit_dependencies = plugin_visit_dependencies,
  .close = plugin_close,
};

struct EntryResolution {
  uint8_t answered;
  uint8_t resolved;
  ttx_abstract requirement;
};

static void TTX_CALL requirement_resolved(
    ttx_requirement_result_self* self,
    ttx_abstract requirement) {
  struct EntryResolution* resolution = (struct EntryResolution*)self;
  resolution->answered = 1;
  resolution->resolved = 1;
  resolution->requirement = requirement;
}

static void TTX_CALL requirement_missing(ttx_requirement_result_self* self) {
  struct EntryResolution* resolution = (struct EntryResolution*)self;
  resolution->answered = 1;
}

static void TTX_CALL requirement_incompatible(
    ttx_requirement_result_self* self,
    const ttx_requirement_descriptor* installed) {
  (void)installed;
  requirement_missing(self);
}

static void TTX_CALL
    requirement_failed(ttx_requirement_result_self* self, ttx_abstract error) {
  (void)error;
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

void TTX_CALL
    ttx_plugin_entry(ttx_host_requirements host, ttx_plugin_entry_sink result) {
  struct EntryResolution resolution = {0};
  ttx_requirement_result requirement_result = {
    .operations = &requirement_result_operations,
    .self = (ttx_requirement_result_self*)&resolution,
  };
  host.operations->resolve(
      host.self, &echo_requirement_descriptor, requirement_result);
  if (!resolution.answered || !resolution.resolved) {
    result.operations->failed(result.self, resolution.requirement);
    return;
  }

  echo_requirement = resolution.requirement;
  plugin_references = 1;
  plugin_closed = 0;
  ttx_plugin plugin = {
    .operations = &plugin_operations,
    .self = (ttx_plugin_self*)&plugin_references,
  };
  result.operations->opened(result.self, plugin);
}
