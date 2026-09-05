// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/plugin/provider_library.hpp"

using namespace Tetrodotoxin;

void Plugin::ProviderLibrary::Error::describe(
    Ttx::Lexical::Errors::Report& report) const {
  report << "Provider library is closed or received another identity."_view;
}

Plugin::ProviderLibrary::ProviderLibrary(
    Ttx::Abstract& selected_provider,
    const ttx_provider_export_descriptor& selected_descriptor)
    : references(1),
      closed(false),
      provider(selected_provider),
      descriptor(selected_descriptor),
      error(),
      operations({
        .header =
            {
              .size = sizeof(ttx_plugin_ops),
              .abi_major = TTX_PLUGIN_ABI_MAJOR,
              .abi_minor = TTX_PLUGIN_ABI_MINOR,
            },
        .retain = retain,
        .release = release,
        .visit_providers = visit_providers,
        .visit_dependencies = visit_dependencies,
        .close = close,
      }) {}

auto Plugin::ProviderLibrary::select(ttx_plugin_self* self)
    -> ProviderLibrary& {
  return *reinterpret_cast<ProviderLibrary*>(self);
}

void Plugin::ProviderLibrary::retain(ttx_plugin_self* self) {
  select(self).references.fetch_add(1, std::memory_order_relaxed);
}

void Plugin::ProviderLibrary::release(ttx_plugin_self* self) {
  select(self).references.fetch_sub(1, std::memory_order_acq_rel);
}

void Plugin::ProviderLibrary::visit_providers(
    ttx_plugin_self* self,
    ttx_provider_sink result) {
  ProviderLibrary& library = select(self);
  if (library.closed) {
    result.operations->failed(result.self, library.error.get_abi());
    return;
  }
  result.operations->provider(
      result.self, &library.descriptor, library.provider.get_abi());
  result.operations->completed(result.self);
}

void Plugin::ProviderLibrary::visit_dependencies(
    ttx_plugin_self* self,
    ttx_abstract candidate,
    ttx_requirement_sink result) {
  ProviderLibrary& library = select(self);
  if (library.closed ||
      !ttx_abstract_same(candidate, library.provider.get_abi())) {
    result.operations->failed(result.self, library.error.get_abi());
    return;
  }
  result.operations->completed(result.self);
}

void Plugin::ProviderLibrary::close(
    ttx_plugin_self* self,
    ttx_plugin_close_sink result) {
  select(self).closed = true;
  result.operations->closed(result.self);
}

auto Plugin::ProviderLibrary::handle() -> ttx_plugin {
  return {
    .operations = &operations,
    .self = reinterpret_cast<ttx_plugin_self*>(this),
  };
}

void Plugin::ProviderLibrary::open(ttx_plugin_entry_sink result) {
  closed = false;
  result.operations->opened(result.self, handle());
}
