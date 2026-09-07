// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/plugin/dialect_library.hpp"

using namespace Tetrodotoxin;

void Plugin::DialectLibrary::Error::describe(
    Ttx::Lexical::Errors::Report& report) const {
  report
      << "The requested Dialect provider is closed or does not own this export."_view;
}

Plugin::DialectLibrary::DialectLibrary(
    Language::Dialect& selected_dialect,
    const ttx_provider_export_descriptor& selected_descriptor,
    const ttx_requirement_descriptor* selected_dependencies,
    size_t selected_dependency_count)
    : references(1),
      closed(false),
      dialect(selected_dialect),
      descriptor(selected_descriptor),
      dependencies(selected_dependencies),
      dependency_count(selected_dependency_count),
      error(),
      plugin_operations({
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
        .close = close,
      }),
      provider_operations({
        .header =
            {
              .size = sizeof(tetrodotoxin_dialect_provider_ops),
              .abi_major = TTX_ABI_MAJOR,
              .abi_minor = TTX_ABI_MINOR,
            },
        .retain = provider_retain,
        .release = provider_release,
        .candidate = candidate,
        .name = name,
        .interpret = interpret,
      }) {}

auto Plugin::DialectLibrary::select(ttx_plugin_self* self) -> DialectLibrary& {
  return *reinterpret_cast<DialectLibrary*>(self);
}

auto Plugin::DialectLibrary::select(tetrodotoxin_dialect_provider_self* self)
    -> DialectLibrary& {
  return *reinterpret_cast<DialectLibrary*>(self);
}

void Plugin::DialectLibrary::plugin_retain(ttx_plugin_self* self) {
  select(self).references.fetch_add(1, std::memory_order_relaxed);
}

void Plugin::DialectLibrary::plugin_release(ttx_plugin_self* self) {
  select(self).references.fetch_sub(1, std::memory_order_acq_rel);
}

void Plugin::DialectLibrary::visit_providers(
    ttx_plugin_self* self,
    ttx_provider_sink result) {
  DialectLibrary& library = select(self);
  if (library.closed) {
    result.operations->failed(result.self, library.error.get_abi());
    return;
  }
  result.operations->dialect(
      result.self, &library.descriptor, library.provider_handle());
  result.operations->completed(result.self);
}

void Plugin::DialectLibrary::visit_dependencies(
    ttx_plugin_self* self,
    ttx_abstract candidate,
    ttx_requirement_sink result) {
  DialectLibrary& library = select(self);
  if (library.closed ||
      !ttx_abstract_same(candidate, library.dialect.get_handle())) {
    result.operations->failed(result.self, library.error.get_abi());
    return;
  }
  for (size_t index = 0; index < library.dependency_count; ++index) {
    result.operations->requirement(result.self, &library.dependencies[index]);
  }
  result.operations->completed(result.self);
}

void Plugin::DialectLibrary::close(
    ttx_plugin_self* self,
    ttx_plugin_close_sink result) {
  select(self).closed = true;
  result.operations->closed(result.self);
}

void Plugin::DialectLibrary::provider_retain(
    tetrodotoxin_dialect_provider_self* self) {
  select(self).references.fetch_add(1, std::memory_order_relaxed);
}

void Plugin::DialectLibrary::provider_release(
    tetrodotoxin_dialect_provider_self* self) {
  select(self).references.fetch_sub(1, std::memory_order_acq_rel);
}

auto Plugin::DialectLibrary::candidate(tetrodotoxin_dialect_provider_self* self)
    -> ttx_abstract {
  return select(self).dialect.get_handle();
}

auto Plugin::DialectLibrary::name(tetrodotoxin_dialect_provider_self* self)
    -> ttx_borrowed_bytes {
  const auto value = select(self).dialect.get_name();
  return {.data = value.get_data(), .size = value.get_size()};
}

void Plugin::DialectLibrary::interpret(
    tetrodotoxin_dialect_provider_self* self,
    tetrodotoxin_source_input input,
    ttx_abstract context,
    tetrodotoxin_interpret_result result) {
  auto& library = select(self);
  if (library.closed) {
    result.operations->failed(result.self, library.error.get_abi());
    return;
  }
  const auto provider = library.dialect.get_provider();
  provider.operations->interpret(provider.self, input, context, result);
}

auto Plugin::DialectLibrary::plugin_handle() -> ttx_plugin {
  return {
    .operations = &plugin_operations,
    .self = reinterpret_cast<ttx_plugin_self*>(this),
  };
}

auto Plugin::DialectLibrary::provider_handle()
    -> tetrodotoxin_dialect_provider {
  return {
    .operations = &provider_operations,
    .self = reinterpret_cast<tetrodotoxin_dialect_provider_self*>(this),
  };
}

void Plugin::DialectLibrary::open(ttx_plugin_entry_sink result) {
  closed = false;
  result.operations->opened(result.self, plugin_handle());
}
