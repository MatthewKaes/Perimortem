// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/app/dialect.hpp"
#include "tetrodotoxin/plugin/dialect_library.hpp"

static constexpr uint8_t package_name[] = "Tetrodotoxin.App";
static constexpr uint8_t export_name[] = "Dialect";
static constexpr uint8_t export_version[] = "1.0";

static const ttx_provider_export_descriptor descriptor = {
  .header =
      {
        .size = sizeof(ttx_provider_export_descriptor),
        .abi_major = TTX_PLUGIN_ABI_MAJOR,
        .abi_minor = TTX_PLUGIN_ABI_MINOR,
      },
  .package_coordinate = {package_name, sizeof(package_name) - 1},
  .exported_route = {export_name, sizeof(export_name) - 1},
  .export_version = {export_version, sizeof(export_version) - 1},
  .content_sha256 = {0x41},
};

static Tetrodotoxin::App::Dialect dialect;
static Tetrodotoxin::Plugin::DialectLibrary plugin(dialect, descriptor);

extern "C" TTX_EXPORT void TTX_CALL
    ttx_plugin_entry(ttx_host_requirements, ttx_plugin_entry_sink result) {
  plugin.open(result);
}
