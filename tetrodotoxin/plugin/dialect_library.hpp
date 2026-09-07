// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <atomic>
#include <cstddef>

#include "tetrodotoxin/language/dialect.hpp"
#include "tetrodotoxin/language/error.hpp"
#include "tetrodotoxin/plugin/abi.h"

namespace Tetrodotoxin::Plugin {

// DialectLibrary gives one existing C++ Dialect the canonical provider and
// plugin handles expected by Environment. The plugin continues to own the
// Dialect object while source construction crosses the same provider operations
// used by foreign frontends. Hosts need no native recovery operation.
class DialectLibrary {
 public:
  DialectLibrary(
      Tetrodotoxin::Language::Dialect& dialect,
      const ttx_provider_export_descriptor& descriptor,
      const ttx_requirement_descriptor* dependencies = nullptr,
      size_t dependency_count = 0);

  DialectLibrary(const DialectLibrary&) = delete;
  DialectLibrary(DialectLibrary&&) = delete;
  auto operator=(const DialectLibrary&) -> DialectLibrary& = delete;
  auto operator=(DialectLibrary&&) -> DialectLibrary& = delete;

  void open(ttx_plugin_entry_sink result);

 private:
  class Error final : public Tetrodotoxin::Language::Error {
   public:
    TTX_NAME("Dialect provider error"_view);
    void describe(Ttx::Lexical::Errors::Report& report) const override;
  };

  static auto select(ttx_plugin_self* self) -> DialectLibrary&;
  static auto select(tetrodotoxin_dialect_provider_self* self)
      -> DialectLibrary&;
  static void TTX_CALL plugin_retain(ttx_plugin_self* self);
  static void TTX_CALL plugin_release(ttx_plugin_self* self);
  static void TTX_CALL
      visit_providers(ttx_plugin_self* self, ttx_provider_sink result);
  static void TTX_CALL visit_dependencies(
      ttx_plugin_self* self,
      ttx_abstract candidate,
      ttx_requirement_sink result);
  static void TTX_CALL
      close(ttx_plugin_self* self, ttx_plugin_close_sink result);
  static void TTX_CALL
      provider_retain(tetrodotoxin_dialect_provider_self* self);
  static void TTX_CALL
      provider_release(tetrodotoxin_dialect_provider_self* self);
  static auto TTX_CALL candidate(tetrodotoxin_dialect_provider_self* self)
      -> ttx_abstract;
  static auto TTX_CALL name(tetrodotoxin_dialect_provider_self* self)
      -> ttx_borrowed_bytes;
  static void TTX_CALL interpret(
      tetrodotoxin_dialect_provider_self* self,
      tetrodotoxin_source_input source,
      ttx_abstract context,
      tetrodotoxin_interpret_result result);

  auto plugin_handle() -> ttx_plugin;
  auto provider_handle() -> tetrodotoxin_dialect_provider;

  std::atomic<uint64_t> references;
  bool closed;
  Tetrodotoxin::Language::Dialect& dialect;
  const ttx_provider_export_descriptor& descriptor;
  const ttx_requirement_descriptor* dependencies;
  size_t dependency_count;
  Error error;
  ttx_plugin_ops plugin_operations;
  tetrodotoxin_dialect_provider_ops provider_operations;
};

}  // namespace Tetrodotoxin::Plugin
