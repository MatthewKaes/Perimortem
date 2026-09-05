// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <atomic>

#include "tetrodotoxin/language/error.hpp"
#include "tetrodotoxin/plugin/abi.h"
#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Plugin {

// ProviderLibrary publishes one ordinary Abstract from a retained shared
// library. It covers providers whose useful operations are negotiated through
// their own contracts, leaving Dialect and Terminal handles to their dedicated
// typed exports.
class ProviderLibrary {
 public:
  ProviderLibrary(
      Ttx::Abstract& provider,
      const ttx_provider_export_descriptor& descriptor);

  ProviderLibrary(const ProviderLibrary&) = delete;
  ProviderLibrary(ProviderLibrary&&) = delete;
  auto operator=(const ProviderLibrary&) -> ProviderLibrary& = delete;
  auto operator=(ProviderLibrary&&) -> ProviderLibrary& = delete;

  void open(ttx_plugin_entry_sink result);

 private:
  class Error final : public Tetrodotoxin::Language::Error {
   public:
    TTX_NAME("Provider error"_view);
    void describe(Ttx::Lexical::Errors::Report& report) const override;
  };

  static auto select(ttx_plugin_self* self) -> ProviderLibrary&;
  static void TTX_CALL retain(ttx_plugin_self* self);
  static void TTX_CALL release(ttx_plugin_self* self);
  static void TTX_CALL
      visit_providers(ttx_plugin_self* self, ttx_provider_sink result);
  static void TTX_CALL visit_dependencies(
      ttx_plugin_self* self,
      ttx_abstract candidate,
      ttx_requirement_sink result);
  static void TTX_CALL
      close(ttx_plugin_self* self, ttx_plugin_close_sink result);

  auto handle() -> ttx_plugin;

  std::atomic<uint64_t> references;
  bool closed;
  Ttx::Abstract& provider;
  const ttx_provider_export_descriptor& descriptor;
  Error error;
  ttx_plugin_ops operations;
};

}  // namespace Tetrodotoxin::Plugin
